/******************************************************************************* 
 * File: bclock.c
 * Description: A simple program to change the MPU clock of the AM335x soc. 
 * 
 * Date: 26/9 2012
 * Author: Dennis Johansson, dennis.johansson@gmail.com
 *******************************************************************************/

/* This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "bclock.h"

void set_dpll(Dpll_mpu* dpll_mpu, uint8_t* map)
{	
	// Set the M_multiplier and N_divider
	if ((dpll_mpu-> M_multiplier  != *(uint16_t *)(map + CM_CLKSEL_DPLL_MPU+1) & 0x3ff)
		|| (dpll_mpu-> N_divider != map[CM_CLKSEL_DPLL_MPU] & 0x7f)
	) {	
			map[CM_CLKMODE_DPLL_MPU_DPLL_EN] &= ~(3); //Clear
			map[CM_CLKMODE_DPLL_MPU_DPLL_EN] |= DPLL_MN_BYP_MODE;
		
			while ( !(map[CM_IDLEST_DPLL_MPU+1] & ST_MN_BYPASS) );
			
			*(uint32_t *) (map + CM_CLKSEL_DPLL_MPU) |= (dpll_mpu->M_multiplier << 8);
			map[CM_CLKSEL_DPLL_MPU] = dpll_mpu->N_divider;

			map[CM_CLKMODE_DPLL_MPU_DPLL_EN] |= DPLL_LOCK_MODE;
			while(  !(map[CM_IDLEST_DPLL_MPU] & ST_DPLL_CLK)  );
		}

	// Set the M2_divider
	if (dpll_mpu-> M2_divider !=  map[CM_DIV_M2_DPLL_MPU] & 0x1f )	
			map[CM_DIV_M2_DPLL_MPU] = dpll_mpu->M2_divider;
}

void get_dpll_mpu(Dpll_mpu* dpll_mpu, uint8_t* map)
{
	dpll_mpu-> N_divider = map[CM_CLKSEL_DPLL_MPU] & 0x7f;
	dpll_mpu-> M_multiplier = *(uint16_t *) (map + CM_CLKSEL_DPLL_MPU+1) & 0x3ff;
	dpll_mpu-> M2_divider = map[CM_DIV_M2_DPLL_MPU] & 0x1f;
}

void print_dpll_mpu(Dpll_mpu* dpll_mpu)
{
	double adplls = (1/((double)dpll_mpu-> N_divider+1))* \
	(double) dpll_mpu-> M_multiplier * CLKINP * (1/(double)dpll_mpu-> M2_divider);
	
	printf("CLKINP = 24\nN_divider = %d\nM_multiplier = %d\nM2_divider = %d\n\nMPU frequency = %.2f Mhz\n", \
	dpll_mpu-> N_divider, dpll_mpu-> M_multiplier, dpll_mpu-> M2_divider, adplls);
}

int main(int argc, char **argv)
{
	Dpll_mpu* dpll_mpu = malloc(sizeof(Dpll_mpu));
	int fd = open("/dev/mem", O_RDWR); 
	int i;
	if (fd < 0) {
		perror("Could not open file \n");
		return 1;
	}
	
	uint8_t *map; 
	map = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, OFFSET);
	if (map == MAP_FAILED) {
		perror("mmap failed");
		return 1;
	}
	
	get_dpll_mpu(dpll_mpu, map);
	
	if (argc == 1) {
		printf("Usage:  %s [OPTION...]\n  -N, set N-divider\n  -M, set M-multiplier\n \
 -M2, set M2-divider\n", argv[0]);

		get_dpll_mpu(dpll_mpu, map);
		printf("\nCurrent settnings \n-----------------\n");
		print_dpll_mpu(dpll_mpu);
		printf("\nCalculated by formula CLKINP * [1 / (N+1)] * [M]  * [1/M2]\n\n");
	}
	else {
		for (i = 1; i < argc; i+=2)
		{
			if (argc != i+1) {
				if (strcmp(argv[i],"-N") ==  0)
					if (atoi(argv[i+1]) > 0 && atoi(argv[i+1]) <= 0x7f)
						dpll_mpu-> N_divider = atoi(argv[i+1]);
				if (strcmp(argv[i],"-M") == 0)
					if (atoi(argv[i+1]) > 0 && atoi(argv[i+1]) <= 0x3ff)
						dpll_mpu-> M_multiplier = atoi(argv[i+1]);
				if (strcmp(argv[i],"-M2") == 0)
					if (atoi(argv[i+1]) > 0 && atoi(argv[i+1]) <= 0x1f)
						dpll_mpu-> M2_divider = atoi(argv[i+1]);
			}
		}
		
		set_dpll(dpll_mpu,map);
		get_dpll_mpu(dpll_mpu, map);
		print_dpll_mpu(dpll_mpu);
		
	}
	munmap(map, SIZE);	
	close(fd);
	
	free(dpll_mpu);
	return 0;
}

