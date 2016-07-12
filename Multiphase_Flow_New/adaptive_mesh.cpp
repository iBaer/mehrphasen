/*
 * adaptive_mesh.cpp
 *
 *  Created on: 20.06.2016
 *      Author: pascal
 */

#include "adaptive_mesh.h"

#include "grid.h"
#include "solver.h"
#include "constants.h"
#include <set>

Adaptive_Mesh::Adaptive_Mesh(Solver * solver, Grid *grid, Constants *constants, Time_Step_Calculation* time_calculation) {
	// TODO Auto-generated constructor stub
	this->solver = solver;
	this->grid_main = grid;
	this->constants = constants;

	this->grid_twostep = new Grid(grid_main->grid_size_total[0],grid_main->grid_size_total[1], constants);
	this->grid_doublestep = new Grid(grid_main->grid_size_total[0],grid_main->grid_size_total[1], constants);

	this->time_calculation = time_calculation;
	// TODO: Wegen rekursion noch ändern
	marked_cells = new int[grid_main->cellsgrid_size]();

}

Adaptive_Mesh::~Adaptive_Mesh() {
	// TODO Auto-generated destructor stub
	delete grid_twostep;
	delete grid_doublestep;
	delete[] marked_cells;

}

void Adaptive_Mesh::amr() {
	//TODO: Über Koordinaten statt "Zellnummer"

	if(grid_main->copy_to(grid_twostep) == -1){
		cout << "Failed to copy Main grid";
		exit(EXIT_FAILURE);
	}
	if(grid_main->copy_to(grid_doublestep) == -1){
		cout << "Failed to copy Main grid";
		exit(EXIT_FAILURE);
	}
	// 1. Gröbstes Netz berechnen
	double dt = 0;
	dt = time_calculation->cfl_condition(grid_main);

	solver->calc_method_flux(dt/2,grid_twostep);
	solver->calc_method_flux(dt/2,grid_twostep);
	solver->calc_method_flux(dt,grid_doublestep);

	// 2. Welche Zellen verfeinern
	// Zellen markieren
	double tolerance = 0.1;
	grid_marker(marked_cells,grid_twostep, grid_doublestep, tolerance);


	// 3. Feineres Gitter erzeugen
	int cluster_amount = binary_clustering(marked_cells, grid_main->grid_size_total[0], grid_main->grid_size_total[1]);
	vector<Cluster_Square> clusters;	//TODO: Grid anhängen, das feinere Gitter (Cluster) haben kann
	int with_adjacencies;
	// Initiales Rechteck Cluster bilden
	square_clustering(clusters, marked_cells, grid_main->grid_size_total[0], grid_main->grid_size_total[1], cluster_amount);
	// Bilde größere Rechtecke solange sich Cluster berühren
	do{
		with_adjacencies = cluster_adjacency_check(clusters, marked_cells, grid_main->grid_size_total[0], grid_main->grid_size_total[1], cluster_amount);
		if (with_adjacencies !=0 && cluster_amount>1)
			square_cluster_merge(clusters, marked_cells, grid_main->grid_size_total[0], grid_main->grid_size_total[1], cluster_amount);
		else
			with_adjacencies=0;

	} while(with_adjacencies!=0);

	// Feines Gitter für alle jedes Cluster der Ebene
	for (unsigned int i = 0; i < clusters.size(); i++){
		create_fine_grid(clusters[i], grid_main);	// TODO: grid_main nur auf Level 0!
	}
	// Flussberechnen auf feinen Gitter
	// Boundary Condtions auf durchlässig, etc

	//TODO: Remove
	 ofstream myfile ("example_fineflux0.txt");
	  if (myfile.is_open())
	  {
		for (int y = 0; y < clusters[0].grid_fine->grid_size_total[1]; y++) {
			for (int x = 0; x < clusters[0].grid_fine->grid_size_total[0]; x++) {
				int pos = x + y * clusters[0].grid_fine->grid_size_total[0];
				myfile << x << " " << y << " " << clusters[0].grid_fine->cellsgrid[pos][0] << endl;
			}
		}
	    myfile.close();
	  }
	  else cout << "Unable to open file";
	solver->calc_method_flux(dt/2,clusters[0].grid_fine);
	//TODO: Remove
	 ofstream myfile2 ("example_fineflux1.txt");
	  if (myfile2.is_open())
	  {
		for (int y = 0; y < clusters[0].grid_fine->grid_size_total[1]; y++) {
			for (int x = 0; x < clusters[0].grid_fine->grid_size_total[0]; x++) {
				int pos = x + y * clusters[0].grid_fine->grid_size_total[0];
				myfile2 << x << " " << y << " " << clusters[0].grid_fine->cellsgrid[pos][0] << endl;
			}
		}
	    myfile2.close();
	  }
	  else cout << "Unable to open file";
	solver->calc_method_flux(dt/2,clusters[0].grid_fine);
	//TODO: Remove
	 ofstream myfile3 ("example_fineflux2.txt");
	  if (myfile3.is_open())
	  {
		for (int y = 0; y < clusters[0].grid_fine->grid_size_total[1]; y++) {
			for (int x = 0; x < clusters[0].grid_fine->grid_size_total[0]; x++) {
				int pos = x + y * clusters[0].grid_fine->grid_size_total[0];
				myfile3 << x << " " << y << " " << clusters[0].grid_fine->cellsgrid[pos][0] << endl;
			}
		}
	    myfile3.close();
	  }
	  else cout << "Unable to open file";

	//----------------------------------------------

	// 4. Rekursiv feinere Gitter berechnen

	// 5. Gröberes Gitter mit feinerem Gitter verbessern

	// 6. Next
}

void Adaptive_Mesh::create_fine_grid(Cluster_Square& cluster, Grid* parent_grid) {
	// TODO: Parent_grid vorher zuweisen!?
	// TODO: Feinegitter Werte von Schritt vorher?

	// Feines Gitter mit doppelter Anzahl Zellen
	int grid_size_x = (cluster.pos_x_max - cluster.pos_x_min + 1)*2;
	int grid_size_y = (cluster.pos_y_max - cluster.pos_y_min + 1)*2;

	// Feines Gitter erstellen und Cluster zuweisen
	Grid* adaptive_grid = new Grid(grid_size_x,grid_size_y, constants); //TODO: DELETE
	cluster.grid_fine = adaptive_grid;
	cluster.parent = parent_grid;

	// Mittelwerte Zellwände (ecken und kanten)
	double half_steps[grid_size_x+1][grid_size_y+1][6]; // n,m = grade -> Zellmittelpunkt (grob) || ungrade -> Zelleckpunkt (grob)

	// half_steps[0][0] bei erster grober Zelle (Mittelpunkt), [1][1] bei (oben rechts) Zellecke [n][m] bei Zellmittelpunkt "grobe Zelle oben rechts"
	// Ausnahme bei an oberen und rechten Rand
	int x_half = 0;
	int y_half = 0;

	for (int x = cluster.pos_x_min; x < cluster.pos_x_max; x++) {
		y_half=0;
		for (int y = cluster.pos_y_min; y < cluster.pos_y_max; y++) {	// TODO: CHeck, vielleicht <= ?
			for (int n = 0; n<=5; n++){
				// Zellemitte, rechte Wand, obere Wand, oben rechts Ecke
				half_steps[x_half][y_half][n] = parent_grid->cellsgrid[(x) + (y) * parent_grid->grid_size_total[0]][n];

				half_steps[x_half+1][y_half][n] = 0.5 * (half_steps[x_half][y_half][n]
													+ parent_grid->cellsgrid[(x+1) + (y) * parent_grid->grid_size_total[0]][n]);

				half_steps[x_half][y_half+1][n] = 0.5 * (half_steps[x_half][y_half][n]
													+ parent_grid->cellsgrid[(x) + (y+1) * parent_grid->grid_size_total[0]][n]);

				half_steps[x_half+1][y_half+1][n] = 0.25 * (half_steps[x_half][y_half][n]
														+ parent_grid->cellsgrid[(x) + (y+1) * parent_grid->grid_size_total[0]][n]
														+ parent_grid->cellsgrid[(x+1) + (y) * parent_grid->grid_size_total[0]][n]
														+ parent_grid->cellsgrid[(x+1) + (y+1) * parent_grid->grid_size_total[0]][n]);
			}
			y_half=y_half+2;
		}
		x_half=x_half+2;
	}

	// "oberer Rand" nur Zellhälften nach rechts
	x_half = 0;
	y_half = (cluster.pos_y_max-cluster.pos_y_min)*2;
	for (int x = cluster.pos_x_min; x < cluster.pos_x_max; x++) {
		for (int n = 0; n<=5; n++){
			half_steps[x_half][y_half][n] = parent_grid->cellsgrid[(x) + (cluster.pos_y_max) * parent_grid->grid_size_total[0]][n];
			half_steps[x_half+1][y_half][n] = 0.5 * (half_steps[x_half][y_half][n]
												+ parent_grid->cellsgrid[(x+1) + (cluster.pos_y_max) * parent_grid->grid_size_total[0]][n]);
		}
		x_half=x_half+2;
	}
	for (int n = 0; n<=5; n++){
		half_steps[x_half][y_half][n] = parent_grid->cellsgrid[(cluster.pos_x_max) + (cluster.pos_y_max) * parent_grid->grid_size_total[0]][n];
	}

	// "rechter Rand" nur Zellhälften nach oben
	x_half = (cluster.pos_x_max-cluster.pos_x_min)*2;
	y_half = 0;
	for (int y = cluster.pos_y_min; y < cluster.pos_y_max; y++) {
		for (int n = 0; n<=5; n++){
			half_steps[x_half][y_half][n] = parent_grid->cellsgrid[(cluster.pos_x_max) + (y) * parent_grid->grid_size_total[0]][n];
			half_steps[x_half][y_half+1][n] = 0.5 * (half_steps[x_half][y_half][n]
												+ parent_grid->cellsgrid[(cluster.pos_x_max) + (y+1) * parent_grid->grid_size_total[0]][n]);
		}
		y_half=y_half+2;
	}

	// Feine Zellmitten über gemittlete Werte an "Ecken"
	for (int x = 1; x < adaptive_grid->grid_size_total[0]-1; x++) {
		for (int y = 1; y < adaptive_grid->grid_size_total[1]-1; y++) {
			for (int n = 0; n <= 5; n++) {
				adaptive_grid->cellsgrid[(x) + (y) * adaptive_grid->grid_size_total[0]][n] = 0.25 * (half_steps[x-1][y-1][n] + half_steps[x-1][y][n] + half_steps[x][y-1][n] + half_steps[x][y][n]);
			}
		}
	}

	// Äußerste Geisterzellen "unten" setzen auf benachbarten Zellwert
	for (int y = 1; y < adaptive_grid->grid_size_total[1]-1; y++) {
		for (int n = 0; n <= 5; n++) {
			adaptive_grid->cellsgrid[(0) + (y) * adaptive_grid->grid_size_total[0]][n] = adaptive_grid->cellsgrid[(1) + (y) * adaptive_grid->grid_size_total[0]][n];
		}
	}
	// Äußerste Geisterzellen "oben" setzen auf benachbarten Zellwert
	for (int y = 1; y < adaptive_grid->grid_size_total[1]-1; y++) {
		for (int n = 0; n <= 5; n++) {
			adaptive_grid->cellsgrid[(adaptive_grid->grid_size_total[0]-1) + (y) * adaptive_grid->grid_size_total[0]][n] = adaptive_grid->cellsgrid[(adaptive_grid->grid_size_total[0]-2) + (y) * adaptive_grid->grid_size_total[0]][n];
		}
	}
	// Äußerste Geisterzellen "links" setzen auf benachbarten Zellwert
	for (int x = 0; x < adaptive_grid->grid_size_total[0]; x++) {
		for (int n = 0; n <= 5; n++) {
			adaptive_grid->cellsgrid[(x) + (0) * adaptive_grid->grid_size_total[0]][n] = adaptive_grid->cellsgrid[(x) + (1) * adaptive_grid->grid_size_total[0]][n];
		}
	}
	// Äußerste Geisterzellen "rechts" setzen auf benachbarten Zellwert
	for (int x = 0; x < adaptive_grid->grid_size_total[0]; x++) {
		for (int n = 0; n <= 5; n++) {
			adaptive_grid->cellsgrid[(x) + (adaptive_grid->grid_size_total[1]-1) * adaptive_grid->grid_size_total[0]][n] = adaptive_grid->cellsgrid[(x) + (adaptive_grid->grid_size_total[1]-2) * adaptive_grid->grid_size_total[0]][n];
		}
	}

	//TODO: Remove
	 ofstream myfile ("example_amr.txt");
	  if (myfile.is_open())
	  {
		for (int y = 0; y < adaptive_grid->grid_size_total[1]; y++) {
			for (int x = 0; x < adaptive_grid->grid_size_total[0]; x++) {
				int pos = x + y * adaptive_grid->grid_size_total[0];
				myfile << x << " " << y << " " << adaptive_grid->cellsgrid[pos][0] << endl;
			}
		}
	    myfile.close();
	  }
	  else cout << "Unable to open file";
}

void Adaptive_Mesh::grid_marker(int* &marked_cells, Grid* grid_one, Grid* grid_two, double tolerance){
	int pos = 0;

	for (int y = 0; y < grid_one->grid_size_total[1]; y++) {
		for (int x = 0; x < grid_one->grid_size_total[0]; x++) {
				pos = x + y * grid_one->grid_size_total[0];
				for(int k=0;k<6;k++){
					if(fabs(grid_one->cellsgrid[pos][k]-grid_two->cellsgrid[pos][k]) > tolerance * fabs(grid_one->cellsgrid[pos][k])){
						//cout << "Checked: "<< x<<","<<y<<endl;
						for (int l = y-2; l<=y+2;l++){
							for (int k = x-2; k<=x+2;k++){
								pos = k + l * grid_one->grid_size_total[0];
								if(k<0 || k >= grid_one->grid_size_total[0] || l<0 || l >= grid_one->grid_size_total[1]){
									//cout << "Couldn't check, out of bounds: "<< k<<","<<l<<endl;
									continue;
								}
								marked_cells[pos] = -1;
							}
						}

						break;
					}
				}
			}
	}

	//TODO: Remove
	 ofstream myfile ("example.txt");
	  if (myfile.is_open())
	  {
		for (int y = 0; y < grid_one->grid_size_total[1]; y++) {
			for (int x = 0; x < grid_one->grid_size_total[0]; x++) {
				pos = x + y * grid_one->grid_size_total[0];
				myfile << x << " " << y << " " << marked_cells[pos] << endl;
			}
		}
	    myfile.close();
	  }
	  else cout << "Unable to open file";
}

int Adaptive_Mesh::binary_clustering(int* &marked_cells, int x_max, int y_max){

	//int* clustered_cells = new int[grid_main->cellsgrid_size]();
	int pos = 0;
	int nth_cluster = 0;
	int* grid_size = new int[2];
	grid_size[0] = x_max;
	grid_size[1] = y_max;
	for(int y=0;y<y_max;y++){
		for(int x=0;x<x_max;x++){
			pos = x + y * grid_size[0];
			if(marked_cells[pos]==-1){
				//cout << x << " " << y << " " << marked_cells[pos] << " pos " << pos <<endl;
				nth_cluster = nth_cluster + 1;
				clusterize(marked_cells, grid_size, nth_cluster, x, y);
			}
		}
	}


	//TODO: Remove
	 ofstream myfile ("example_cluster.txt");
	  if (myfile.is_open())
	  {
		for (int y = 0; y < grid_size[1]; y++) {
			for (int x = 0; x < grid_size[0]; x++) {
				pos = x + y * grid_size[0];
				myfile << x << " " << y << " " << marked_cells[pos] << endl;
			}
		}
	    myfile.close();
	  }
	  else cout << "Unable to open file";

		delete[] grid_size;
		//delete[] clustered_cells;
	return nth_cluster;
}

//TODO: Gefahr wegen Rekursion?
void Adaptive_Mesh::clusterize(int* &clustered_cells, int* grid_size, int nth_cluster, int x, int y){
	int pos = x + y * grid_size[0];
	if(x>= 0 && x< grid_size[0] && y>= 0 && y< grid_size[1] && clustered_cells[pos]==-1){
		//cout << "Markingat " << x << "," << y << " | old: " << clustered_cells[pos] << " | new: " << nth_cluster << endl;
		clustered_cells[pos] = nth_cluster;
		clusterize(clustered_cells, grid_size, nth_cluster, x-1, y);
		clusterize(clustered_cells, grid_size, nth_cluster, x, y-1);
		clusterize(clustered_cells, grid_size, nth_cluster, x+1, y);
		clusterize(clustered_cells, grid_size, nth_cluster, x, y+1);
	}
}

void Adaptive_Mesh::square_clustering(vector<Cluster_Square> &clusters, int* &marked_cells, int x_max, int y_max, int cluster_amount){
	int pos = 0;

	for(int n = 1; n <= cluster_amount; n++){
		int first = 1;
		clusters.push_back(Cluster_Square());
		for (int y = 0; y < y_max; y++) {
			for (int x = 0; x < x_max; x++) {

				pos = x + y * x_max;

				if(marked_cells[pos]==n){
					if(first==1){
						clusters.at(n-1).pos_x_min = x;
						clusters.at(n-1).pos_x_max = x;
						clusters.at(n-1).pos_y_min = y;
						clusters.at(n-1).pos_y_max = y;
						first=0;
					}
					else if(x < clusters.at(n-1).pos_x_min)
						clusters.at(n-1).pos_x_min = x;
					else if(x > clusters.at(n-1).pos_x_max)
						clusters.at(n-1).pos_x_max = x;
					else if(y < clusters.at(n-1).pos_y_min)
						clusters.at(n-1).pos_y_min = y;
					else if(y > clusters.at(n-1).pos_y_max)
						clusters.at(n-1).pos_y_max = y;
				}
			}
		}
	}

	// Marking new clusters
	for(int n = 0; n < cluster_amount; n++){
		cout << "Cluster #"<<n+1<<" from "<< clusters.at(n).pos_x_min<<","<<clusters.at(n).pos_y_min<<" to "<< clusters.at(n).pos_x_max<<","<<clusters.at(n).pos_y_max<<endl;
		for (int y = clusters.at(n).pos_y_min; y <= clusters.at(n).pos_y_max; y++) {
			for (int x = clusters.at(n).pos_x_min; x <= clusters.at(n).pos_x_max; x++) {
				pos = x + y * x_max;
				marked_cells[pos] = n+1;
			}
		}
	}

	//TODO: Remove
	 ofstream myfile ("example_squared.txt");
	  if (myfile.is_open())
	  {
		for (int y = 0; y < y_max; y++) {
			for (int x = 0; x < x_max; x++) {
				pos = x + y * x_max;
				myfile << x << " " << y << " " << marked_cells[pos] << endl;
			}
		}
	    myfile.close();
	  }
	  else cout << "Unable to open file";

	  return;

}

int Adaptive_Mesh::cluster_adjacency_check(vector<Cluster_Square> &clusters, int* &marked_cells, int x_max, int y_max, int cluster_amount) {

	int with_adjacencies = 0;
	int y_touch = 0;
	int x_touch = 0;

	for (int n = 0; n < cluster_amount; n++) {
		for (int l = n+1; l < cluster_amount; l++){
			y_touch = 0;
			x_touch = 0;
			if(clusters[n].pos_x_min-1 > clusters[l].pos_x_min && clusters[n].pos_x_min-1 <= clusters[l].pos_x_max)
				x_touch = 1;
			else if(clusters[n].pos_x_max+1 < clusters[l].pos_x_max && clusters[n].pos_x_max+1 >= clusters[l].pos_x_min)
				x_touch = 1;
			else
				continue;
			if(clusters[n].pos_y_min-1 > clusters[l].pos_y_min && clusters[n].pos_y_min-1 <= clusters[l].pos_y_max)
				y_touch = 1;
			else if(clusters[n].pos_y_max+1 < clusters[l].pos_y_max && clusters[n].pos_y_max+1 >= clusters[l].pos_y_min)
				y_touch = 1;
			else
				continue;

			if(x_touch==1 && y_touch==1){
				clusters[n].adjacencies.insert(l);
				with_adjacencies++;
			}
		}
	}

	for (int n = 0; n < cluster_amount; n++) {
		cout << "Cluster #" << n + 1 << " adjacent to" << endl;
		for (set<int>::iterator it = clusters[n].adjacencies.begin(); it != clusters[n].adjacencies.end(); ++it)
			cout << "\t Cluster #" << *it+1 << endl;
	}

	return with_adjacencies;
}

void Adaptive_Mesh::square_cluster_merge(vector<Cluster_Square> &clusters, int* &marked_cells, int x_max, int y_max, int &cluster_amount){

	int* cluster_to_destroy = new int[cluster_amount]();

	for (int n = cluster_amount-1; n >=0; n--) {
		if(clusters[n].adjacencies.begin() != clusters[n].adjacencies.end()){
			for (set<int>::iterator it = clusters[n].adjacencies.begin(); it != clusters[n].adjacencies.end(); ++it){
				clusters[*it].adjacencies.insert(n);
			}
			cluster_to_destroy[n] = 1;
		}
	}

	for (int n = 0; n < cluster_amount; n++) {
		for (set<int>::iterator it = clusters[n].adjacencies.begin(); it != clusters[n].adjacencies.end(); ++it){
			if(clusters[n].pos_x_min > clusters[*it].pos_x_min)
				clusters[n].pos_x_min = clusters[*it].pos_x_min;
			if(clusters[n].pos_x_max < clusters[*it].pos_x_max)
				clusters[n].pos_x_max = clusters[*it].pos_x_max;
			if(clusters[n].pos_y_min > clusters[*it].pos_y_min)
				clusters[n].pos_y_min = clusters[*it].pos_y_min;
			if(clusters[n].pos_y_max < clusters[*it].pos_y_max)
				clusters[n].pos_y_max = clusters[*it].pos_y_max;
		}
		clusters[n].adjacencies.clear();
	}

	for (int n = cluster_amount-1; n >=0; n--) {
		if(cluster_to_destroy[n] == 1){
			clusters.erase(clusters.begin()+n);
			cluster_amount--;
		}
	}

	int pos = 0;
	// Marking new clusters
	for(int n = 0; n < cluster_amount; n++){
		cout << "Cluster #"<<n+1<<" from "<< clusters.at(n).pos_x_min<<","<<clusters.at(n).pos_y_min<<" to "<< clusters.at(n).pos_x_max<<","<<clusters.at(n).pos_y_max<<endl;
		for (int y = clusters.at(n).pos_y_min; y <= clusters.at(n).pos_y_max; y++) {
			for (int x = clusters.at(n).pos_x_min; x <= clusters.at(n).pos_x_max; x++) {
				pos = x + y * x_max;
				marked_cells[pos] = n+1;
			}
		}
	}

	delete[] cluster_to_destroy;

	//TODO: Remove
	 ofstream myfile ("example_squared_big.txt");
	  if (myfile.is_open())
	  {
		for (int y = 0; y < y_max; y++) {
			for (int x = 0; x < x_max; x++) {
				pos = x + y * x_max;
				myfile << x << " " << y << " " << marked_cells[pos] << endl;
			}
		}
	    myfile.close();
	  }
	  else cout << "Unable to open file";
}

/*void Adaptive_Mesh::cluster_adjacency_check(int* &marked_cells, int x_max, int y_max, int cluster_amount, int** cluster_locations, set<int>* &adjacencies) {

	int pos = 0;
	int y = 0;
	int x = 0;
	for (int n = 0; n < cluster_amount; n++) {
		y = -1;
		while (y != cluster_locations[n][3]) {
			if (y == -1) {
				y = cluster_locations[n][2];
			} else if (y == cluster_locations[n][2]) {
				y = cluster_locations[n][3];
			}
			for (x = cluster_locations[n][0]; x <= cluster_locations[n][1]; x++) {
				if (y == cluster_locations[n][2]) {
					if (y - 1 >= 0) { // eventuell nur größer
						pos = x + (y - 1) * x_max;
						if (n + 1 != marked_cells[pos] && marked_cells[pos] > 0) {
							adjacencies[n].insert(marked_cells[pos]);

							if (adjacencies[n].find(marked_cells[pos]) == adjacencies[n].end())
							 adjacencies[n].insert(marked_cells[pos]);
							 }
						} else {
							break;
						}
					} else if (y == cluster_locations[n][3]) {
						if (y + 1 <= y_max) { // eventuell nur kleiner
							pos = x + (y + 1) * x_max;
							if (n + 1 != marked_cells[pos] && marked_cells[pos] > 0) {
								adjacencies[n].insert(marked_cells[pos]);

							}
						} else {
							break;
						}
					}
				}
			}

			x = -1;
			while (x != cluster_locations[n][1]) {
				if (x == -1) {
					x = cluster_locations[n][0];
				} else if (x == cluster_locations[n][0]) {
					x = cluster_locations[n][1];
				}
				for (int y = cluster_locations[n][2]; y <= cluster_locations[n][3]; y++) {
					if (x == cluster_locations[n][0]) {
						if (x - 1 >= 0) { // eventuell nur größer
							pos = (x - 1) + y * x_max;
							if (n + 1 != marked_cells[pos] && marked_cells[pos] > 0) {
								adjacencies[n].insert(marked_cells[pos]);

							}
						} else {
							break;
						}
					} else if (x == cluster_locations[n][1]) {
						if (x + 1 <= x_max) { // eventuell nur kleiner
							pos = (x + 1) + y * x_max;
							if (n + 1 != marked_cells[pos] && marked_cells[pos] > 0) {
								adjacencies[n].insert(marked_cells[pos]);

							}
						} else {
							break;
						}
					}
				}
			}
		}
	}

	for (int n = 0; n < cluster_amount; n++) {
		cout << "Cluster #" << n + 1 << " adjacent to" << endl;
		for (std::set<int>::iterator it = adjacencies[n].begin(); it != adjacencies[n].end(); ++it)
			std::cout << "\t Cluster #" << *it << endl;
	}

	return;
}*/