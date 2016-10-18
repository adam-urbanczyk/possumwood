#include <iostream>
#include <string>
#include <stdexcept>

#include <boost/program_options.hpp>

#include <GL/freeglut.h>

#include <QApplication>
#include <QMainWindow>

#include <ImathVec.h>

#include "bind.h"
#include "node.h"
#include "graph_widget.h"

namespace po = boost::program_options;

using std::cout;
using std::endl;
using std::flush;

int main(int argc, char* argv[]) {
	// // Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
	("help", "produce help message")
	;

	// process the options
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if(vm.count("help")) {
		cout << desc << "\n";
		return 1;
	}

	///////////////////////////////

	QApplication app(argc, argv);

	// make a window only with a viewport
	QMainWindow win;

	win.showMaximized();

	///

	GraphWidget* graph = new GraphWidget(&win);
	win.setCentralWidget(graph);

	Node& n1 = graph->addNode("first", QPointF(-50, -20), {{"aaaaa", Port::kInput}, {"b", Port::kOutput}});
	Node& n2 = graph->addNode("second", QPointF(50, -20), {{"xxxxxxxxxxxxxxxx", Port::kInputOutput}});

	graph->connect(n1.port(1), n2.port(0));

	///

	app.exec();

	return 0;
}
