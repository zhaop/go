#!/usr/bin/env python3

import argparse
import atexit
import fcntl
import os
import re
import subprocess
import sys


OK = 0
ERROR = 1
QUIT = 2

# Convert gtp-color /b(lack)?|w(hite)?/i to engine-color /1|2/
def engine_color(s):
	matches = re.search(r'^(?:(b(?:lack)?)|(w(?:hite)?))$', s, re.I)
	if not matches:
		raise ValueError('could not parse gtp-color: {}'.format(s))

	groups = matches.groups()
	if groups[0]:
		return '1'
	elif groups[1]:
		return '2'
	else:
		raise ValueError('could not convert gtp-color to engine-color: {}'.format(s))

# Convert gtp-vertex /pass|([a-hj-t]{1-19})/i to engine-move
# Note: gtp("a1") is bottom-left and should be engine("80") on a 9x9,
#       but for simplicity we convert it as engine("00") instead.
#       So the engine sees a "top-bottom mirrored" version of the board,
#       which for most purposes is equivalent.
def engine_vertex(s):
	s = s.lower()

	if s == 'pass':
		return '--'

	col, row = s[0], s[1:]

	col_letters = 'abcdefghjklmnopqrstuvwxyz'	# a-z without i
	if col not in col_letters:
		raise ValueError('could not parse gtp-vertex column {} in {}'.format(repr(col), repr(s)))
	j = col_letters.index(col)

	try:
		i = int(row, 10) - 1
	except ValueError:
		raise ValueError('could not parse gtp-vertex row {} in {}'.format(repr(row), repr(s)))

	if i < 0:
		raise ValueError('gtp-vertex row {} must be non-negative in {}'.format(repr(row), repr(s)))

	return '{}{}'.format(i, j)

def gtp_vertex(s):
	if s == '--':
		return 'pass'

	i, j = int(s[0], 36), int(s[1], 36)

	col_letters = 'abcdefghjklmnopqrstuvwxyz'	# a-z without i
	if j >= len(col_letters):
		raise ValueError('could not convert engine-vertex column {} to gtp-vertex column: out-of-range'.format(repr(col)))

	row = str(i + 1)
	col = col_letters[j]

	return '{}{}'.format(col, row)

class GtpWrapper:

	known_cmds = {
		'protocol_version',
		'name',
		'version',
		'known_command',
		'list_commands',
		'quit',
		'boardsize',
		'clear_board',
		'komi',
		'play',
		'genmove',
		'showboard',
	}

	def __init__(self, engine_path, debug=False, log_file=None):
		self.engine = subprocess.Popen([engine_path, '-c'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		self.debug = debug
		self.log_file = log_file
		atexit.register(self._cleanup)

	def _cleanup(self):
		if self.engine.poll() is not None:
			result = self.engine.wait(1)
			if result is None:
				self.engine.terminate()

	def _set_blocking(self, fd):
		flags = fcntl.fcntl(fd, fcntl.F_GETFL) & ~os.O_NONBLOCK
		fcntl.fcntl(fd, fcntl.F_SETFL, flags)

	def _set_nonblocking(self, fd):
		flags = fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK
		fcntl.fcntl(fd, fcntl.F_SETFL, flags)

	def _read_until(self, stream, sentinel):
		self._set_nonblocking(stream)
		buf = type(sentinel)()
		while True:
			data = stream.read(1)
			if data:
				buf += data
				if buf.endswith(sentinel):
					break
		self._set_blocking(stream)
		return buf

	def _log(self, msg):
		if self.log_file:
			self.log_file.write(msg)
			self.log_file.flush()

	# cmd: 1 line or multiple lines of engine console command, WITHOUT final line break(s)
	def call_engine(self, cmd, multiline=False):
		engine = self.engine

		# Clear stdout
		self._read_until(engine.stdout, b'> ')

		self._log('< {}\n'.format(cmd))

		engine.stdin.write((cmd + '\n').encode())
		engine.stdin.flush()

		if multiline:
			reply = self._read_until(engine.stdout, b'\n\n').decode()
			reply = reply.rstrip('\n')

		else:
			reply = engine.stdout.readline().decode().rstrip('\n')

		for line in reply.split('\n'):
			self._log('> {}\n'.format(line))

		return reply

	def cmd_protocol_version(self):
		return OK, 2

	def cmd_name(self):
		return OK, 'Teresa'

	def cmd_version(self):
		return OK, '1.0'

	def cmd_known_command(self, cmd):
		if self.is_known_command(cmd):
			return OK, 'true'
		else:
			return OK, 'false'

	def cmd_list_commands(self):
		cmd_names = filter(self.is_known_command, GtpWrapper.known_cmds)
		return OK, '\n'.join(sorted(cmd_names))

	def cmd_quit(self):
		return QUIT, ''

	def cmd_boardsize(self, size_str):
		try:
			size = int(size_str, 10)
		except ValueError:
			return ERROR, 'syntax error'

		if size != 9:
			return ERROR, 'unacceptable size'

		# TODO Send a command when dynamic board sizes are supported

		return OK, ''

	def cmd_clear_board(self):
		cmd = 'c'

		result = self.call_engine(cmd)

		return OK, ''

	def cmd_komi(self, komi):
		try:
			cmd = 'k {}'.format(float(komi))
		except ValueError:
			return ERROR, 'syntax error'

		result = self.call_engine(cmd)

		if result.startswith('!syntax') or result.startswith('!invalid'):
			return ERROR, 'syntax error'

		return OK, ''

	def cmd_play(self, color, vertex):
		try:
			cmd = 'p {} {}'.format(engine_color(color), engine_vertex(vertex))
		except ValueError:
			return ERROR, 'syntax error'

		result = self.call_engine(cmd)

		if result.startswith('!player'):
			return ERROR, 'playing out-of-turn not supported'
		elif result.startswith('!move'):
			return ERROR, 'syntax error'
		elif result.startswith('!illegal'):
			return ERROR, 'illegal move'
		elif result.startswith('!result'):
			return ERROR, 'engine error: {}'.format(result.replace('!result: ', ''))

		return OK, ''

	def cmd_genmove(self, color):
		try:
			cmd = 'g {}'.format(engine_color(color))
		except ValueError:
			return ERROR, 'syntax error'

		result = self.call_engine(cmd)

		if result.startswith('!player'):
			return ERROR, 'playing out-of-turn not supported'
		elif result.startswith('!result'):
			return ERROR, 'engine error: {}'.format(result.replace('!result: ', ''))

		if result == ':/':
			return OK, 'resign'
		else:
			return OK, gtp_vertex(result)

	def cmd_showboard(self):
		result = self.call_engine('dg', multiline=True)
		return OK, '\n' + result

	def preprocess(self, line):
		# [9, 10, 32, ..., 126]
		whitelist = ''.join(chr(i) for i in [9, 10] + list(range(32, 127)))

		# Keep only printable non-control ASCII
		clean = ''.join(filter(lambda x: x in whitelist, line))

		# Remove comments
		if '#' in clean:
			clean = clean[:clean.index('#')]

		# Replace tabs
		clean = clean.replace('\t', ' ')

		# Misc cleanup
		clean = clean.strip()

		return clean

	# Parse a command into (id, command_name, arguments), with
	# - id: int, or None
	# - command_name: str
	# - arguments: list(str)
	def parse(self, command):
		parts = command.split(' ')

		if parts[0].isnumeric():
			# Has id
			return (int(parts[0]), parts[1], parts[2:])
		else:
			# No id
			return (None, parts[0], parts[1:])

	def is_known_command(self, cmd):
		return (cmd in GtpWrapper.known_cmds) and hasattr(self, 'cmd_' + cmd) and callable(getattr(self, 'cmd_' + cmd))

	def run(self, line):
		command = self.preprocess(line)

		if not command:
			return

		cid, cmd, cargs = self.parse(command)

		if self.is_known_command(cmd):
			status, out = getattr(self, 'cmd_' + cmd)(*cargs)

			if out:
				out = ' {}'.format(out)
			print('{}{}{}'.format(['=', '?', '='][status], cid or '', out))

			if status == QUIT:
				sys.exit(0)

		else:
			print('?{} unknown command'.format(cid or ''))

		print()


def main():
	global wrapper

	parser = argparse.ArgumentParser()
	parser.add_argument('engine', nargs='?', default='./go-teresa-9x9', help='path to game engine')
	parser.add_argument('-d', action='store_true', help='debug mode')
	parser.add_argument('-l', nargs='?', help='path to log file')
	args = parser.parse_args()

	if args.l:
		wrapper = GtpWrapper(args.engine, debug=args.d, log_file=open(args.l, 'a'))
	else:
		wrapper = GtpWrapper(args.engine, debug=args.d)

	while True:
		line = input()
		wrapper.run(line)

if __name__ == '__main__':
	main()