#include "constants.h"

#include <fstream>
#include <string>
#include <stdlib.h>

using namespace std;

/**
 *****************************************************************************************
 * Konstruktor.
 * @param input_const String des Pfades zu den festen Konstanten.
 * @param input_calc String des Pfades zu den einmalig zu berechnenden Konstanten.
 *****************************************************************************************/
Constants::Constants() {
	input_const = "gas-liquid.in";
	ifstream input(input_const.c_str());
	string line, name, calc;
	double value;

	if (!input_const.empty()) {
		if (input.is_open()) {
			//einlesen Leerzeichen als Trenzeichen
			while (getline(input, line)) {
				name = line.substr(0, line.find(" "));
				value = atof(
						line.substr(line.find(" ") + 1,
								line.size() - line.find(" ") - 1).c_str());
				if (name == "calceigv") {
					calceigv = value;
					cout << "Wert von calceigv ist " << value << endl;
				}
				if (name == "variante")
					variante = value;
				if (name == "teiler")
					teiler = value;
				if (name == "teilerend")
					teilerend = value;
				if (name == "timeou")
					timeou = value;
				if (name == "cfl")
					cfl = value;
				if (name == "maxnt")
					maxnt = value;
				if (name == "dimension")
					dimension = value;
				if (name == "ordnung")
					order = value;
				if (name == "radius")
					radius = value;
				if (name == "CELLSX")
					grid_size_x = value;
				if (name == "CELLSY")
					grid_size_y = value;
				if (name == "g")
					gamma = value;
				if (name == "mol")
					pos_x_min = value;
				if (name == "mor")
					pos_x_max = value;
				if (name == "mul")
					pos_y_min = value;
				if (name == "mur")
					pos_y_max = value;
				if (name == "upbc")
					bc_y_max = value;
				if (name == "downbc")
					bc_y_min = value;
				if (name == "leftbc")
					bc_x_min = value;
				if (name == "rightbc")
					bc_x_max = value;
				if (name == "cref")
					cref = value;
				if (name == "done")
					done = value;
				if (name == "ccl")
					ccl = value;
				if (name == "rhol")
					rhol = value;
				if (name == "vl")
					vl = value;
				if (name == "vrl")
					vrl = value;
				if (name == "vyl")
					vyl = value;
				if (name == "vyrl")
					vyrl = value;
				if (name == "rhor")
					rhor = value;
				if (name == "vr")
					vr = value;
				if (name == "vrr")
					vrr = value;
				if (name == "vyr")
					vyr = value;
				if (name == "vyrr")
					vyrr = value;

				//const_name.push_back(name);
				//const_value.push_back(value);
			}
			input.close();
		} else {
			cout << "Fehler beim öffnen der Konstanten!";
		}
	}

    double alfll = 1.0 - rhol/done + ccl *(rhol/done);
    double dll = ccl * (rhol/alfll);
    double pll = cref*pow(dll,gamma);

    ct = pll/pow(rhol,gamma);

}

Constants& Constants::instance() {
	static Constants constants;
	return constants;
}
