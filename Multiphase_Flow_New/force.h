#ifndef FORCE_H
#define FORCE_H

#include "numerische_methode.h"
#include "solver.h"
/*!
* @class FORCE
* Klasse der FORCE Methode.
*/


class Force : public Solver
{
    public:
        /**
        * Konstruktor der FORCE Methode.
        * Ruft den Konstruktor der geerbten Klasse auf.
        * @param const_in Dateiname wo Konstanten gespeichert sind.
        * @param formel_in Dateinamen-Kern für die Formeln.
        * @param save_in Dateiname wo für das Laden eine Speicherstands die Plots gespeichert sind.
        */
        Force(Constants *constants, Computation *computation, Grid *grid);
        virtual ~Force();

    protected:
        /**
        * Berechnung des FORCE Flusses.
        * @return 4 Dimensionaler Vektor. Zusammenstellung: Gleichung, x-Position, y-Position , dimension
        */
        double* calc_method_flux(double dt,int dir);

  	  //int size_total[0];
  	  //int size_total[1];
  	  int neqs;

  	  double *uall;
  	  double *fall;
  	  double *gall;
  	  double *f_laxall;
  	  double *f_rieall;
  	  double *g_laxall;
  	  double *g_rieall;

  	  double *** cs;
  	  double *** fd;
  	  double *** gd;
  	  double *** f_lax;
  	  double *** f_rie;
  	  double *** g_lax;
  	  double *** g_rie;

  	  double** f_force;


};

#endif // FORCE_H