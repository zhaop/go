// Adapted from http://jsfiddle.net/Nivaldo/CbGh2/

var diameter_obsolete = 1000;
var height_obsolete = diameter_obsolete;

var i = 0,
	duration = 350,
	root;

var json_path = ({"#1": "graph1.json", "#2": "graph2.json", "#3": "graph3.json", "#4": "graph4.json"})[location.hash] || "graph1.json";
document.title = ({"#1": "Black pre-move", "#2": "Black post-move", "#3": "White pre-move", "#4": "White post-move"})[location.hash] || "Black pre-move";

function player_name(player) {
	return ({"b": "black", "w": "white"})[player];
}

var tree = d3.layout.tree()
	.size([360, diameter_obsolete / 2 - 80])
	.separation(function(a, b) { return (a.parent == b.parent ? 1 : 10) / a.depth; });

var diagonal = d3.svg.diagonal.radial()
	.projection(function(d) { return [d.y, d.x / 180 * Math.PI]; });

var svg = d3.select("body").append("svg")
	.attr("id", "vis")
	.append("g")
	.attr("transform", "translate(" + document.documentElement.clientWidth/2 + "," + document.documentElement.clientHeight/2 + ")");

d3.json(json_path, function (error, gr) {
	if (error) throw error;

	root = gr;
	root.x0 = height_obsolete / 2;
	root.y0 = 0;

	function collapse(d) {
	  if (d.children) {
		  d._children = d.children;
		  d._children.forEach(collapse);
		  d.children = null;
		}
	}

	// root.children.forEach(collapse);
	update(root);
});

d3.select(self.frameElement).style("height", "800px");

function update(source) {

	// Compute the new tree layout.
	var nodes = tree.nodes(root).reverse(),
		links = tree.links(nodes);

	// Normalize for fixed-depth.
	nodes.forEach(function(d) { d.y = d.depth * 120; });

	// Update the nodes...
	var node = svg.selectAll("g.node")
		.data(nodes, function(d) { return d.id || (d.id = ++i); });

	// Enter any new nodes at the parent's previous position.
	var nodeEnter = node.enter()
		.append("g")
		.attr("class", "node")
		.classed("black", function (d) { return d.player == "b" })
		.classed("white", function (d) { return d.player == "w" })
		.attr("title", function(d) { return d.player + d.move + " : " + d.id.replace('0x','') })
		// .attr("transform", function(d) { return "rotate(" + (d.x - 90) + ")translate(" + d.y + ")"; })///
		.on("click", click);

	nodeEnter.append("circle")
		.attr("r", 1e-6)
		.classed("parent", function(d) { return d._children; });
		// .style("fill", function(d) { return d._children ? "lightsteelblue" : "#fff"; });

	nodeEnter.append("text")
		.attr("x", 10)
		.attr("dy", ".35em")
		.attr("text-anchor", "start")
		// .attr("transform", function(d) { return d.x < 180 ? "translate(0)" : "rotate(180)translate(-" + (Math.log10(d.visits) * 8.5)  + ")"; })///
		.text(function(d) { return d.visits + ' (' + Math.round(d.wins/d.visits*1000)/10 + ')'; })
		.style("fill-opacity", 1e-6);

	// Transition nodes to their new position.
	var nodeUpdate = node.classed("parent", function(d) { return d._children; })
		.transition()
		.duration(duration)
		.attr("transform", function(d) { return "rotate(" + (d.x - 90) + ")translate(" + d.y + ")"; })

	nodeUpdate.select("circle")
		.attr("r", function(d) { return Math.log10(d.visits)*1.5 + 1; })
		// .style("fill", function(d) { return d._children ? "lightsteelblue" : "#fff"; });

	nodeUpdate.select("text")
		.style("fill-opacity", 1)
		.attr("transform", function(d) { return d.x < 180 ? "translate(0)" : "rotate(180)translate(-" + (this.textContent.length*5.6 + 40)  + ")"; });

	// TODO: appropriate transform
	var nodeExit = node.exit().transition()
		.duration(duration)
		.attr("transform", function(d) { return "diagonal(" + source.y + "," + source.x + ")"; }) ///
		.remove();

	nodeExit.select("circle")
		.attr("r", 1e-6);

	nodeExit.select("text")
		.style("fill-opacity", 1e-6);

	// Update the links…
	var link = svg.selectAll("path.link")
		.data(links, function(d) { return d.target.id; });

	// Enter any new links at the parent's previous position.
	link.enter().insert("path", "g")
		.attr("class", "link")
		.attr("d", function(d) {
		  var o = {x: source.x0, y: source.y0};
		  return diagonal({source: o, target: o});
		})
		.attr("stroke-opacity", function(d) { return 0.98 * d.target.visits / d.source.visits + 0.02; })
		.attr("stroke-width", function(d) { return Math.pow(Math.log10(d.target.visits), 2) / 1.5; });

	// Transition links to their new position.
	link.transition()
		.duration(duration)
		.attr("d", diagonal);

	// Transition exiting nodes to the parent's new position.
	link.exit().transition()
		.duration(duration)
		.attr("d", function(d) {
		  var o = {x: source.x, y: source.y};
		  return diagonal({source: o, target: o});
		})
		.remove();

	// Stash the old positions for transition.
	nodes.forEach(function(d) {
	  d.x0 = d.x;
	  d.y0 = d.y;
	});
}

// Toggle children on click.
function click(d) {
  if (d.children) {
	d._children = d.children;
	d.children = null;
  } else {
	d.children = d._children;
	d._children = null;
  }
  
  update(d);
}