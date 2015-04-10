/**
 * @file 	integrator.c
 * @brief 	Hybrid symplectic/IAS15 integration scheme.
 * @author 	Hanno Rein <hanno@hanno-rein.de>
 * @detail	This file implements the leap-frog integration scheme.  
 * This scheme is second order accurate, symplectic and well suited for 
 * non-rotating coordinate systems. Note that the scheme is formally only
 * first order accurate when velocity dependent forces are present.
 * 
 * @section 	LICENSE
 * Copyright (c) 2015 Hanno Rein
 *
 * This file is part of rebound.
 *
 * rebound is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rebound is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rebound.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "particle.h"
#include "main.h"
#include "gravity.h"
#include "boundaries.h"
#include "integrator.h"
#include "integrator_mikkola.h"
#include "integrator_ias15.h"

double integrator_hybrid_switch_ratio = 10.; // Switch to non-symplectic integrator if force_form_star/force_from_other_particle < integrator_hybrid_switch_ratio 
static double initial_dt = 0;


static double get_min_ratio(){
	struct particle p0 = particles[0];
	const int _N_active = ((N_active==-1)?N:N_active)- N_megno;
	const int _N_real   = N - N_megno;
	double min_ratio = 1e308;
	for (int i=1; i<_N_active; i++){
		struct particle pi = particles[i];
	for (int j=1; j<_N_real; j++){
		if (i==j) continue;
		const double dxj = p0.x - particles[j].x;
		const double dyj = p0.y - particles[j].y;
		const double dzj = p0.z - particles[j].z;
		const double r0j2 = dxj*dxj + dyj*dyj + dzj*dzj;

		const double dx = pi.x - particles[j].x;
		const double dy = pi.y - particles[j].y;
		const double dz = pi.z - particles[j].z;
		const double rij2 = dx*dx + dy*dy + dz*dz;
		
		const double F0j = p0.m/r0j2;
		const double Fij = pi.m/rij2;

		const double ratio = F0j/Fij;
			
		if (ratio<min_ratio){
			min_ratio = ratio;
		}
	}
	}
	return min_ratio;
}


static double ratio;
unsigned int integrator_hybrid_mode = 0; // 0 = symplectic; 1 = IAS15
void integrator_hybrid_part1(){
	ratio = get_min_ratio();
	if (initial_dt==0.){
		initial_dt = dt;
	}
	if (ratio<integrator_hybrid_switch_ratio){
		if (integrator_hybrid_mode==0){
			integrator_ias15_reset(); //previous guesses no good anymore
			gravity_ignore_10 = 0;
		}
		integrator_hybrid_mode = 1;
	}else{
		if (integrator_hybrid_mode==1){
			//integrator_mikkola_reset(); 
			dt = initial_dt;
		}
		integrator_hybrid_mode = 0;
	}
	switch(integrator_hybrid_mode){
		case 0:
			integrator_mikkola_part1();
			break;
		case 1:
			integrator_ias15_part1();
			break;
	}
}
void integrator_hybrid_part2(){
	switch(integrator_hybrid_mode){
		case 0:
			integrator_mikkola_part2();
			break;
		case 1:
			integrator_ias15_part2();
			break;
	}
}
	
void integrator_hybrid_synchronize(){
	//integrator_mikkola_synchronize();
}

void integrator_hybrid_reset(){
	integrator_hybrid_mode = 0;
	integrator_mikkola_reset();
	integrator_ias15_reset();
	integrator_hybrid_switch_ratio = 10.;
	initial_dt = 0.;
}
