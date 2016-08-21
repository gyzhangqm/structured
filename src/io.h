#ifndef _IO_H
#define _IO_H
#include "common.h"
#include "utils.h"
#include "mesh.h"
#include "config.h"

class IOManager{
public:
	Config *config;
	Mesh<double> *mesh;
	double *xc_array, *yc_array, *q_array;
	IOManager(Mesh<double> *val_mesh, Config *val_config){
		config = val_config;
		mesh = val_mesh;

		// required for npz
		uint nic = mesh->nic;
		uint njc = mesh->njc;
		uint nq = mesh->solution->nq;
		
		xc_array = new double[nic*njc];
		yc_array = new double[nic*njc];
		q_array = new double[nic*njc*nq];
	};

	~IOManager(){
		delete[] xc_array;
		delete[] yc_array;
		delete[] q_array;
		
	}

	void write(uint iteration){
		if(iteration%config->io->fileout_frequency == 0){
			write_tecplot();
			write_npz();
			write_restart();
			write_surface();
		}
	}
	
	void write_tecplot(){
		std::string filename = config->io->label + ".tec";
		uint nic = mesh->nic;
		uint njc = mesh->njc;
		double***q = mesh->solution->q;
		double**xc = mesh->xc;
		double**yc = mesh->yc;
		
		std::ofstream outfile;
		outfile.open(filename);
		char buffer [500];
		outfile<<"title = \"Solution\""<<"\n";
		outfile<<"variables = \"x\" \"y\" \"rho\" \"u\" \"v\" \"p\""<<"\n"; 
		outfile<<"zone i="<<nic<<", j="<<njc<<", f=point\n";
		for(uint j=0; j<njc; j++){
			for(uint i=0; i<nic; i++){
				sprintf(buffer, "%8.14E %8.14E %8.14E %8.14E %8.14E %8.14E\n", xc[i][j], yc[i][j], q[i][j][0], q[i][j][1], q[i][j][2], q[i][j][3]);
				outfile<<buffer;
			}
		}
		
		outfile.close();
		spdlog::get("console")->info("Wrote tecplot file {}.", filename);
	}
	
	
	void write_npz(){
		std::string filename = config->io->label + ".npz";
		uint nic = mesh->nic;
		uint njc = mesh->njc;
		double***q = mesh->solution->q;
		double**xc = mesh->xc;
		double**yc = mesh->yc;
		const unsigned int shape[] = {nic, njc};
		const unsigned int shapeq[] = {nic, njc, 4};
		
		const unsigned int shapetmp[] = {5U, 2U};
			
		for(int i=0; i<nic; i++){
			for(int j=0; j<njc; j++){
				xc_array[i*njc + j] = xc[i][j];
				yc_array[i*njc + j] = yc[i][j];
				for(int k=0; k<4; k++){
					q_array[i*njc*4 + j*4 + k] = q[i][j][k];
				}
			}
		}
		cnpy::npz_save(filename,"xc",xc_array,shape,2,"w");
		cnpy::npz_save(filename,"yc",yc_array,shape,2,"a");
		cnpy::npz_save(filename,"q",q_array,shapeq,3,"a");
		
		spdlog::get("console")->info("Wrote numpy npz file {}.", filename);
	}


	void write_restart(){
		std::string filename = config->io->label + ".out";
		uint nic = mesh->nic;
		uint njc = mesh->njc;
		uint nq = mesh->solution->nq;
		double ***q = mesh->solution->q;
		double **xc = mesh->xc;
		double **yc = mesh->yc;
		std::ofstream outfile(filename,std::ofstream::binary);
		for(int i=0; i<nic; i++){
			for(int j=0; j<njc; j++){
				outfile.write(reinterpret_cast<const char*>(q[i][j]), sizeof(double)*nq);
			}
		}
		outfile.close();
		spdlog::get("console")->info("Wrote restart file {}.", filename);
	}

	void read_restart(){
		std::string filename = config->io->label + ".out";
		uint nic = mesh->nic;
		uint njc = mesh->njc;
		uint nq = mesh->solution->nq;
		double***q = mesh->solution->q;
		double**xc = mesh->xc;
		double**yc = mesh->yc;
		std::ifstream infile(filename,std::ofstream::binary);

		infile.seekg (0,infile.end);
		long size = infile.tellg();
		long size_expected = nic*njc*nq*sizeof(double);
		infile.seekg (0);
		assert(size == size_expected);
		
		for(int i=0; i<nic; i++){
			for(int j=0; j<njc; j++){
				infile.read(reinterpret_cast<char*>(q[i][j]), sizeof(double)*nq);
			}
		}
		infile.close();
		
	}
	void write_surface(){
		std::string filename = config->io->label + ".surface";
		uint nic = mesh->nic;
		uint njc = mesh->njc;
		uint nq = mesh->solution->nq;
		double***q = mesh->solution->q;
		double**xc = mesh->xc;
		double**yc = mesh->yc;
		double p_inf = 1/1.4;
		double rho_inf = 1.0;
		double u_inf = 0.5;
		int j1 = mesh->j1-1;
		double rho, u, v, p, x, cp;
		std::ofstream outfile;
		outfile.open(filename);
		
		for(uint i=j1; i<j1+mesh->nb; i++){
			primvars<double>(q[i][0], &rho, &u, &v, &p);
			x = xc[i][0];
			cp = (p - p_inf)/(0.5*rho_inf*u_inf*u_inf);
			outfile<<x<<" "<<cp<<std::endl;
		}
		outfile.close();
		spdlog::get("console")->info("Wrote surface file {}.", filename);
	}
};

#endif