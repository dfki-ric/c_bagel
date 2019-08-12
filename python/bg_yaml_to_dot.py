#!/usr/bin/env python

import sys
import os
import yaml

if len(sys.argv) < 2:
    print "no argument given"
    exit(0)

if not os.path.isfile(sys.argv[1]):
    print "argument is not a correct file path"
    exit(0)

filename = sys.argv[1]
ymlGraph = yaml.load(open(filename))

s = '''digraph "%s" {
  size="100,100";
  ranksep="1";
  nodesep="1";
''' %filename

lineWidth = 2.0
for node in ymlGraph["nodes"]:
    nodeId = int(node["id"])
    s += "  node%05d" %nodeId
    s += " [penwidth=%f, shape=Mrecord, label=\"[%d]|{" %(lineWidth, nodeId)

    idx = 0
    for nInput in node["inputs"]:
        if idx == 0:
            s += "{"
        else:
            s += "| "
        first = False
        s += " <i%d> %s b(%5.2f) d(%5.2f)" %(int(nInput["idx"]), nInput["type"], nInput["bias"], nInput["default"])
        idx += 1
    s += "} |<o> %s}\"" %node["type"].replace(">", "\>")

    if node["type"] == "INPUT":
        s += ", fillcolor=\"#bbeebb\", style=filled"
    elif node["type"] == "OUTPUT":
        s += ", fillcolor=\"#bbccff\", style=filled"
    else:
        s += ", fillcolor=\"#ffff70\", style=filled"

    s += "];\n"

s += "\n"

for edge in ymlGraph["edges"]:
    fromId = int(edge["fromNodeId"])
    fromIdx = int(edge["fromNodeOutputIdx"])
    toId = int(edge["toNodeId"])
    toIdx = int(edge["toNodeInputIdx"])
    weight = float(edge["weight"])

    s += "node%05d:o -> node%05d:i%d [penwidth=%f, label=\"%5.2f\"];\n" %(fromId, toId, toIdx, lineWidth, weight)

s += "}\n"

with open(filename+".dot", "w") as f:
    f.write(s)
