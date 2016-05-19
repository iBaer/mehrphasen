#include "computation.h"
#include "constants.h"
#include "force.h"
#include "lax_friedrich.h"


using namespace std;

int main()
{
	Solver* solver;
	Constants constants = Constants::instance();
    int flux_method = 1, grid_method = 0;
    cout << "#####################################"<<endl;
    cout << "####                             ####"<<endl;
    cout << "####  Multiphase Flow Simulator  ####"<<endl;
    cout << "####                             ####"<<endl;
    cout << "#####################################"<<endl<<endl;

    /*** Wahl der Numerischen Methode ***/
    cout << "Please select the numerical method for flux calculation:"<< endl;
    cout << "[0] Lax-Friedrich"<< endl;
    cout << "[1] FORCE"<< endl;
	cout << "Choice: ";
    cin >> flux_method;
    cout << endl;
    /**********************************/

	cout << constants.dimension << "D grid" << endl;
	if(constants.dimension==1){
		cout << "Simulation area: [" << constants.pos_x_min << "] to [" << constants.pos_x_max << "]" << endl;
		cout << "Simulation grid size: [" << constants.grid_size_x << "]" << endl;
	}
	else if(constants.dimension==2){
		cout << "Simulation area: [" << constants.pos_x_min << "," << constants.pos_y_min << "] to [" << constants.pos_x_max
				<< "," << constants.pos_y_max << "]" << endl;
		cout << "Simulation grid size: [" << constants.grid_size_x << "," << constants.grid_size_y
				<< "]" << endl;
	}
	else if(constants.dimension==3){
		cout << "Not yet implemented"<<endl;
		return -1;
	}

    /*** Wahl der Raster Initialisierungsmethode ***/
	cout << "Grid initialisation:"<<endl;
    switch (constants.dimension) {
    	case (1):
    		cout << "[0] Rarefraction wave"<< endl;
    		cout << "[1] one shock wave !"<< endl;
    		cout << "Choice: ";
    		cin >> grid_method;
    		break;
    	case (2):
			cout << "[0] Rarefraction wave"<< endl;
			cout << "[1] 60 Grad Winkel"<< endl;
			cout << "[2] 45 Grad Winkel"<< endl;
			cout << "[3] Blasen Simulation"<< endl;
			cout << "[4] Speicherstand laden!"<< endl;
			cout << "Choice: ";
			cin >> grid_method;
		    cout << endl;
			break;
    	default:
    		cout << "Invalid dimension"<<endl;
    }
    /**********************************/

    Grid grid = Grid(&constants, "save.in", grid_method);
	Computation computation = Computation::instance(&constants);

    if (flux_method == 0)
    {
        solver = new Lax_Friedrich(&constants,&computation, &grid);
    }
    else if (flux_method == 1)
    {
    	solver = new Force(&constants,&computation, &grid);
    }
    else{
    	cout << "No valid method selected"<<endl<<"Closing Simulator"<<endl;
    	return 0;
    }
    numerische_methode num_meth(solver,&constants,&computation, &grid);

    /*** Wahl der Splitting Methode ***/
	if (constants.dimension >= 2) {
		std::cout << "Splitting updates?" << endl;
		cout << "[1] Unsplitting"<< endl;
		cout << "[2] Splitting"<< endl;
		cout << "Choice: ";
		std::cin >> num_meth.with_splitting;
		cout << endl;
	}
    /**********************************/

    /*** Wahl ob Output pro Schritt ***/
	cout << "Stepwise Output?"<<endl;
	cout << "[0] No"<< endl;
	cout << "[1] Yes"<< endl;
	cout << "Choice: ";
	cin >> num_meth.step_output;
	cout << endl;
    /**********************************/

    num_meth.start_method();

    delete solver;
    return 0;

}
