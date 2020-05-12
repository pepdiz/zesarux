/*
    ZEsarUX  ZX Second-Emulator And Released for UniX
    Copyright (C) 2013 Cesar Hernandez Bano

    This file is part of ZEsarUX.

    ZEsarUX is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "tape.h"
#include "tape_smp.h"
#include "cpu.h"
#include "operaciones.h"
#include "debug.h"
#include "zx8081.h"
#include "snap.h"
#include "screen.h"
#include "menu.h"
#include "snap_zx8081.h"
#include "utils.h"
#include "settings.h"

FILE *ptr_mycinta_smp;

int lee_smp_ya_convertido;

//puntero de lectura de archivo generado en memoria
int spec_smp_read_index_tap;


//puntero de escritura de archivo generado en memoria
int spec_smp_write_index_tap;

//total de bytes de archivo generado en memoria
int spec_smp_total_read;

//donde se guarda el archivo generado en memoria
z80_byte *spec_smp_memory;

//int main_spec_smpatap(void);
int main_spec_rwaatap(void);

//solo para decir que color de texto en oscuro o no
//int tape_guessing_parameters=0;

#define MAX_BYTES_READ (1024*1024)


//Archivo temporal
char inputfile_name_rwa[PATH_MAX];


//si archivo es rwa. lo abre tal cual
//si es smp u otros, lo convierte
void open_input_file(void)
{

	//Inicializar siempre esto. Si no se confunde si ha habido una conversion anterior
	ptr_mycinta_smp=NULL;


	if (!util_compare_file_extension(tapefile,"smp")) {
		if (lee_smp_ya_convertido==0) {
			convert_to_rwa_common_tmp(tapefile,inputfile_name_rwa);
			convert_smp_to_rwa(tapefile,inputfile_name_rwa);
		}

		ptr_mycinta_smp=fopen(inputfile_name_rwa,"rb");
		//printf ("convertido a rwa : %s\n",inputfile_name_rwa);
	}

	else if (!util_compare_file_extension(tapefile,"wav")) {
		if (lee_smp_ya_convertido==0) {
			convert_to_rwa_common_tmp(tapefile,inputfile_name_rwa);
			if (convert_wav_to_rwa(tapefile,inputfile_name_rwa)) {
				debug_printf (VERBOSE_ERR,"Error converting wav to rwa");
				return;
			}
		}
		ptr_mycinta_smp=fopen(inputfile_name_rwa,"rb");
	}


	else {
		ptr_mycinta_smp=fopen(tapefile,"rb");
	}


	lee_smp_ya_convertido=1;


}

//funcion para escribir un byte en memoria, comprobando si se sale de limite
//retorna 0 si ok
//1 si error
int spec_smp_write_mem_byte(int index,z80_byte valor)
{
	if (index>=MAX_BYTES_READ) {
		return 1;
	}

	spec_smp_memory[index]=valor;
	return 0;
}


int tape_block_smp_open(void)
{



	if (!(MACHINE_IS_SPECTRUM)) return 0;


	else {
		//avisar que no se ha abierto aun el archivo. Esto se hace porque en la rutina de Autodetectar,
		//cada vez se abre el archivo de nuevo, y evitar que se tenga que convertir (por ejemplo de wav) una y otra vez
		lee_smp_ya_convertido=0;

		open_input_file();

		if (!ptr_mycinta_smp)
		{
			debug_printf(VERBOSE_ERR,"Unable to open input file %s",tapefile);
			tapefile=0;
			return 1;
		}



		main_spec_rwaatap();


		return 0;
	}
}


int tape_block_smp_read(void *dir,int longitud)
{

	if (spec_smp_read_index_tap>=spec_smp_total_read) {
		debug_printf(VERBOSE_INFO,"End of file");
		return 0;
	}


	memcpy(dir,&spec_smp_memory[spec_smp_read_index_tap],longitud);

	spec_smp_read_index_tap +=longitud;

	return longitud;



}

int tape_block_smp_readlength(void)
{
	z80_byte buffer[2];
	if (spec_smp_read_index_tap>=spec_smp_total_read) {
		debug_printf(VERBOSE_INFO,"End of file");
		return 0;
	}

	memcpy(buffer,&spec_smp_memory[spec_smp_read_index_tap],2);
	spec_smp_read_index_tap +=2;

	//printf ("tape_block_smp_readlength: %d\n",value_8_to_16(buffer[1],buffer[0]));

	return value_8_to_16(buffer[1],buffer[0]);

}


int tape_block_smp_seek(int longitud,int direccion)
{

	switch (direccion) {
		case SEEK_CUR:
			spec_smp_read_index_tap +=longitud;
			break;

		default:
			debug_printf (VERBOSE_ERR,"tape_block_smp_seek. whence invalid : %d",direccion);
			return -1;
			break;
	}

	return 0;

}





//Cargar en RAM datos obtenidos del audio de SMP
void snap_load_zx80_zx81_load_smp(void)
{
	main_leezx81();
}



//
//Rutina de conversion de audio de sonido de carga de spectrum a binario
//


/*
 * Ficheros .P del emulador XTENDER:
 * Contienen los bytes tal cual fueron grabados en el ZX81
 * exceptuando los bytes del nombre, que vienen al principio, y
 * el byte del nombre final tiene el bit 7 alzado
 */


unsigned int zx8081_fic_leido;

int zx8081_sensibilidad_cambio,zx8081_longitud_cambio;


char *fichero,*fichero_p;

//SMP 11111 hz
//char zx8081_ceros=18,zx8081_unos=33;

//RWA 15600 hz
char zx8081_ceros=25,zx8081_unos=46;

unsigned char zx8081_byte_cambio;
char zx8081_final_fichero=0;

int zx8081_debugonda=0;




int zx8081_da_abs(int valor)
{
	if (valor>=0) return valor;
	else return -valor;
}

int zx8081_lee_byte(void)
//Funcion que lee byte del fichero
//Pone zx8081_final_fichero a 1 si se llega al final del fichero
{

	zx8081_byte_cambio=fgetc(ptr_mycinta_smp);
	zx8081_fic_leido++;
	//printf (" %d\n",zx8081_fic_leido);

	if (feof(ptr_mycinta_smp)) {
		zx8081_final_fichero=1;
		return 0;
	}

	return zx8081_byte_cambio;
}

int zx8081_lee_onda(unsigned char *longitud)
//Funcion que lee una onda completa de sonido
//Devuelve -1 si se llega al final del fichero
//Se lee a 11111hz, 8 bit, mono, unsigned
{


	int debug_leidos=0;
	unsigned char veces;
	int byte,byte_ant;

	//Primero posicionarse en una onda de sonido
	//Ver si la onda cambia bruscamente (mas de zx8081_sensibilidad_cambio) en mas de zx8081_longitud_cambio bytes

	//printf ("Sensibilidad cambio: %d\n",zx8081_sensibilidad_cambio);


	byte_ant=zx8081_lee_byte();

	veces=0;

	do {
		if (zx8081_debugonda) printf ("S");
		if (zx8081_final_fichero) {
			debug_printf (VERBOSE_DEBUG,"End audio input file waiting audio value high change");
			//printf ("longitud esperando cambio: %d leidos: %d\n",*longitud,debug_leidos);
			return -1;
		}
		byte=zx8081_lee_byte();
		debug_leidos++;

		//Parche para soportar conversiones no muy buenas de smp a rwa, en que se repite el ultimo byte de vez en cuando
		//Desactivado Parche, pues entonces lo que sucede es que con archivos rwa
		//generados mediante save del emulador en zx81, los silencios entre bits tienen mismo valor, y se interpretarian
		//como parte de este parche
		//if (byte==byte_ant) {
		//}

		//else {
		int diferencia;
		diferencia=zx8081_da_abs(byte-byte_ant);
		//printf ("esperando cambio brusco: byte antes: %d byte despues: %d diferencia: %d sensibilidad cambio: %d veces: %d zx8081_longitud_cambio: %d\n",byte_ant,byte,diferencia,zx8081_sensibilidad_cambio,veces,zx8081_longitud_cambio);

		if (diferencia>=zx8081_sensibilidad_cambio) veces++;
		else {
			//en vez de invalidar esto (veces=0) decir que "la ultima" no cuenta
			if (veces) veces--;
		}

		//}


		byte_ant=byte;
	} while (veces<zx8081_longitud_cambio);



	*longitud=veces+1;
	//printf ("longitud despues de esperar cambio: %d leidos: %d\n",*longitud,debug_leidos);
	debug_leidos=0;

	//A partir de ahora leer la longitud hasta que el cambio no sea brusco

	//valor zx8081_longitud_cambio depende de frecuencia muestreo
	//valor zx8081_sensibilidad_cambio depende del volumen y/o bits (8 o 16)

	//Se tiene byte
	veces=0;

	byte_ant=byte;
	do {
		if (zx8081_debugonda) printf ("O");
		byte=zx8081_lee_byte();
		if (zx8081_final_fichero) {
			debug_printf (VERBOSE_DEBUG,"End file reading data. Length: %d",*longitud);
			//printf ("fin de archivo. longitud: %d leidos: %d\n",*longitud,debug_leidos);

			return -1;
		}

		//Parche para soportar conversiones no muy buenas de smp a rwa, en que se repite el ultimo byte de vez en cuando
		//Desactivado Parche, pues entonces lo que sucede es que con archivos rwa
		//generados mediante save del emulador en zx81, los silencios entre bits tienen mismo valor, y se interpretarian
		//como parte de este parche
		//if (byte==byte_ant) {
		//	(*longitud)--;
		//}

		//else {
		int diferencia;
		diferencia=zx8081_da_abs(byte-byte_ant);
		//printf ("esperando cambio no brusco byte antes: %d byte despues: %d diferencia: %d sensibilidad cambio: %d veces: %d zx8081_longitud_cambio: %d\n",byte_ant,byte,diferencia,zx8081_sensibilidad_cambio,veces,zx8081_longitud_cambio);
		if (diferencia<zx8081_sensibilidad_cambio) {
			veces++;
			if (veces>=zx8081_longitud_cambio) break;
		}
		else {
			//en vez de invalidar esto (veces=0) decir que "la ultima" no cuenta
			if (veces) veces--;
		}

		//}

		(*longitud)++;
		byte_ant=byte;
		debug_leidos++;
	} while (1);


	//printf ("longitud despues de cambio no brusco: %d leidos: %d\n",*longitud,debug_leidos);

	return 0;

}

int zx8081_dice_margen(int n,int valor,int izq,int der)
//Funcion que dice si el valor n esta entre [valor-izq,valor+der]
{

	return (n>=valor-izq && n<=valor+der);

}

int zx8081_dice_bit(char numero)
//Dice si el bit es 0 o 1 segun su numero de ondas
//Devuelve -1 si no es un bit aceptado
{
	if (zx8081_dice_margen(numero,zx8081_ceros,12,12)) {
		if (zx8081_debugonda) printf ("0");
		return 0;
	}
	if (zx8081_dice_margen(numero,zx8081_unos,12,12)) {
		if (zx8081_debugonda) printf ("1");
		return 1;
	}
	unsigned int n=(unsigned int)numero;
	debug_printf (VERBOSE_DEBUG,"Value %d for a bit length not accepted",n);
	return -1;

}

int zx8081_lee_1_bit(void)
//Funcion que lee 1 bit
{

	unsigned char longitud;

	if (zx8081_lee_onda(&longitud)==-1) return -1;

	return zx8081_dice_bit(longitud);
}

int zx8081_lee_8_bits(void)
//Devuelve 8 bits leidos
//Devuelve -1 si se llega al final de los datos
{
	char bit;
	int n,byte=0;

	for (n=0;n<8;n++) {
		bit=zx8081_lee_1_bit();
		//printf ("bit: %d ",bit);
		if (bit==-1) return -1;
		byte=byte*2+bit;
	}
	return byte;
}

int zx8081_escribe_nombre(unsigned char *m,int leidos)
//Funcion que escribe el nombre del fichero y retorna la longitud del nombre
{
	unsigned char n;
	int l=0;
	z80_bit inverse;

	do {
		if (!leidos) break;
		n=*m++;
		leidos--;
		l++;
		putchar(da_codigo81(n,&inverse));
	} while (n<128);


	return l;

}

int zx8081_escribe_nombre_to_string(unsigned char *m,unsigned char *s,int leidos)
//Funcion que escribe el nombre del fichero en una string y retorna la longitud del nombre
{
	unsigned char n;
	int l=0;
	z80_bit inverse;
	z80_byte caracter;

	do {
		if (!leidos) break;
		n=*m++;
		leidos--;
		l++;
		if (l>255) {
			debug_printf (VERBOSE_INFO,"Error. Name is bigger than 255 bytes");
			return l;
		}

		caracter=da_codigo81(n,&inverse);
		*s++=caracter;
		//putchar(da_codigo81(n,&inverse));
	} while (n<128);

	*s=0;
	return l;

}



int zx8081_lee_todos_bytes(unsigned char *m)
{




	open_input_file();

	if (!ptr_mycinta_smp)
	{
		debug_printf(VERBOSE_ERR,"Unable to open input file %s",tapefile);
		tapefile=0;
		return -1;
	}



	zx8081_fic_leido=0;
	zx8081_final_fichero=0;


	int retorno;
	int bytes_leidos=0;
	unsigned char byte_leido;

	do {
		retorno=zx8081_lee_8_bits(/*-1*/);
		if (retorno==-1) break;

		byte_leido=retorno;

		*m++=byte_leido;
		bytes_leidos++;
	} while (1);


	fclose(ptr_mycinta_smp);

	return bytes_leidos;

}


void main_leezx81(void)
//int main_leezx81(int argc,char *argv[])
{

	int bytes_leidos;
	int auto_parametros=0;

	unsigned char *buffer_memoria;
	unsigned char *buffer_memoria_orig;


	zx8081_sensibilidad_cambio=3;

	//11111 hz
	//zx8081_longitud_cambio=3;

	//15600 hz
	//zx8081_longitud_cambio=4;
	//temp
	zx8081_longitud_cambio=3;




	auto_parametros=1;



	//Mensaje de aviso que se esta procesando

	//borrar texto             01234567890123456789012345678901
	menu_putstring_footer(0,2,"                                ",WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);

	//color inverso
	menu_putstring_footer(0,2,"Guessing Loading Parameters...",WINDOW_FOOTER_PAPER,WINDOW_FOOTER_INK);



	//si no hay este cpuloop, no se refresca la pantalla en xwindows
	int conta;
	for (conta=0;conta<20000;conta++) {
		new_snap_load_zx8081_simulate_cpuloop();
	}
	scr_refresca_pantalla();



	//Asignar memoria
	if ((buffer_memoria=(unsigned char *)malloc(65536L))==NULL) {
		cpu_panic ("Error allocating memory when reading smp file");
	}

	buffer_memoria_orig=buffer_memoria;


	debug_printf (VERBOSE_DEBUG,"Reading smp audio data...");


	//avisar que no se ha abierto aun el archivo. Esto se hace porque en la rutina de Autodetectar,
	//cada vez se abre el archivo de nuevo, y evitar que se tenga que convertir (por ejemplo de wav) una y otra vez
	lee_smp_ya_convertido=0;

	if (auto_parametros==0) {
		bytes_leidos=zx8081_lee_todos_bytes(buffer_memoria);
		if (bytes_leidos==-1) {
			//Error
			return;
		}
	}


	else {
		int i;
		int mejor_zx8081_sensibilidad_cambio=2;
		int mejor_bytes_leidos=0;
		int mejor_zx8081_fic_leido=0;

		zx8081_sensibilidad_cambio=2;

		debug_printf (VERBOSE_INFO,"Autodetecting best loading parameters...");

		//30 diferentes valores de zx8081_sensibilidad_cambio
		for (i=0;i<30;i++) {
			debug_printf (VERBOSE_DEBUG,"Testing with Threshold of wave change: %d",zx8081_sensibilidad_cambio);
			bytes_leidos=zx8081_lee_todos_bytes(buffer_memoria);

			if (bytes_leidos==-1) {
				//Error
				return;
			}

			debug_printf (VERBOSE_DEBUG,"Bytes read: %d",bytes_leidos);
			if (bytes_leidos>mejor_bytes_leidos) {
				mejor_bytes_leidos=bytes_leidos;
				mejor_zx8081_sensibilidad_cambio=zx8081_sensibilidad_cambio;
				mejor_zx8081_fic_leido=zx8081_fic_leido;
			}

			zx8081_sensibilidad_cambio++;
		}

		debug_printf (VERBOSE_DEBUG,"Best Threshold of wave change: %d Bytes read: %d Sound Bytes read: %d",mejor_zx8081_sensibilidad_cambio,mejor_bytes_leidos,mejor_zx8081_fic_leido);

		//Relanzamos lectura con el mejor parametro de sensibilidad
		zx8081_sensibilidad_cambio=mejor_zx8081_sensibilidad_cambio;
		//printf ("sensi: %d\n",zx8081_sensibilidad_cambio);
		bytes_leidos=zx8081_lee_todos_bytes(buffer_memoria);
		debug_printf (VERBOSE_DEBUG,"Bytes read: %d",bytes_leidos);


	}



	//borrar texto             01234567890123456789012345678901
	menu_putstring_footer(0,2,"                                ",WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);

	menu_footer_bottom_line();



	if (bytes_leidos) {


		if (verbose_level>=VERBOSE_DEBUG) {
			//mostrar por consola
			int i;
			z80_bit inverse;

			printf ("Data loaded:\n");

			for (i=0;i<bytes_leidos;i++) printf ("%c",da_codigo81(buffer_memoria[i],&inverse));

			printf ("\n");

		}


		if (MACHINE_IS_ZX81) {

			z80_byte buffer_nombre[257];
			int longitud_nombre=zx8081_escribe_nombre_to_string(buffer_memoria,buffer_nombre,bytes_leidos);
			debug_printf (VERBOSE_INFO,"Total bytes read: %d Program name length: %d Program name: %s",bytes_leidos,longitud_nombre,buffer_nombre);


			//Descartar nombre
			bytes_leidos -=longitud_nombre;
			buffer_memoria +=longitud_nombre;

		}

		else {
			debug_printf (VERBOSE_INFO,"Total bytes read: %d",bytes_leidos);
		}

		debug_printf (VERBOSE_INFO,"Sound Bytes read: %u Program length (without the name):%u ",
			      zx8081_fic_leido,bytes_leidos);
		if (bytes_leidos) {

			z80_int offset_destino;

			offset_destino=0;

			if (MACHINE_IS_ZX81) offset_destino=0x4009;
			if (MACHINE_IS_ZX80) offset_destino=0x4000;

			if (offset_destino==0) cpu_panic ("Destination dir is zero");


			if (offset_destino+bytes_leidos>ramtop_zx8081) debug_printf (VERBOSE_ERR,"Read bytes (%d) over ramtop (%d)",bytes_leidos,ramtop_zx8081);
			//printf ("offset_destino: %d\n",offset_destino);



			if (tape_loading_simulate.v==1) {
				new_snap_load_zx80_zx81_simulate_loading(memoria_spectrum+offset_destino,buffer_memoria,bytes_leidos);
			}

			//Igualmente lo leemos, aunque traspase ramtop
			memcpy(memoria_spectrum+offset_destino,buffer_memoria,bytes_leidos);
		}
	}
	if (!bytes_leidos) debug_printf (VERBOSE_ERR,"Error: Program length is zero");

	free(buffer_memoria_orig);



}





//
//Rutina de conversion de audio de sonido de carga de spectrum a binario
//

//de smpatap spectrum

//11111 hz
#define SPEC_NO_RUIDO 		2


#define SPEC_ONDAS_GUIA 	10


char *spec_tipos_fichero[]={
	"Program",
	"Number Array",
	"Character Array",
	"Bytes",
	"Flag"
};

//int spec_ondas_leidas;



unsigned char spec_carry;

//unsigned char *memoria;
//unsigned char *memoria_original;
unsigned int spec_bytes_leidos;

//char fichero_smp[1024],fichero_tap[1024];
char *fichero_smp,*fichero_tap;

//Para smp 11111
//char spec_tono_guia=14,spec_ceros=6,spec_unos=12,spec_mitad_onda_falsa=6;
//char margen_spec_tono_guia=2;

//Para rwa 15600
char spec_tono_guia=20,spec_ceros=8,spec_unos=16,spec_mitad_onda_falsa=8;
char margen_spec_tono_guia=3;

//Para smp 11111
//char margen_spec_ceros=2;
//char margen_spec_unos=2;

//Para rwa 15600
char margen_spec_ceros=3;
char margen_spec_unos=3;



char spec_byte_cambio,spec_cambio=0;
char spec_final_fichero=0;

int spec_da_ascii(int codigo)
{
	return (codigo<127 && codigo>31 ? codigo : '.');
}

int spec_da_abs(int valor)
{
	if (valor>=0) return valor;
	else return -valor;
}


int tempp=0;

int spec_lee_byte(void)
//Funcion que lee byte del fichero
//Mira si hay un byte de cambio de onda, en cuyo caso lo devuelve
//Pone spec_final_fichero a 1 si se llega al final del fichero
{
	if (spec_cambio) {
		spec_cambio=0;
		return spec_byte_cambio;
	}

	else {
		spec_byte_cambio=fgetc(ptr_mycinta_smp);
		tempp++;
		//printf ("l: %d\n",tempp);
		//unsigned char v;
		//v=spec_byte_cambio;
		// printf ("%x ",v);
	}

	if (feof(ptr_mycinta_smp)) {
		spec_final_fichero=1;
		return 0;
	}

	//printf ("leido: %d\n",spec_byte_cambio);

	return spec_byte_cambio;
}

char spec_da_signo(char valor)
//Devuelve el signo de valor: -1,+1 o 0
{

	if (valor>=0) return 1;
	if (valor<0) return -1;

	//TODO: aqui se llega alguna vez?
	return 0;
}

int spec_lee_onda(unsigned char *longitud,unsigned char *amplitud)
//Funcion que lee una onda completa de sonido
//Da la maxima amplitud (en positivo) y la longitud
//de esa onda
//Devuelve -1 si se llega al final del fichero
{
	char byte,byte_anterior,veces=0;

	*longitud=1;
	*amplitud=0;

	byte_anterior=spec_lee_byte();
	*amplitud=spec_da_abs(byte_anterior);


	if (spec_final_fichero) return -1;

	do {
		byte=spec_lee_byte();
		if (spec_final_fichero) return -1;

		if (spec_da_abs(byte)>*amplitud) *amplitud=spec_da_abs(byte);

		//printf ("amplitud: %d\n",*amplitud);

		if (spec_da_signo(byte)!=spec_da_signo(byte_anterior)
			//&& spec_da_abs(byte)>=SPEC_NO_RUIDO
		) {

			if (veces==1) {
				spec_cambio=1;

				//printf ("cambio signo con longitud: %u\n",*longitud);

				return 0;
			}

			veces++;
		}
		(*longitud)++;
		byte_anterior=byte;
	} while (1);
}

int spec_dice_bit(char longitud)
//Dice si el bit es 0 o 1 segun su amplitud
//Devuelve -1 si no es un bit aceptado
{
	if (longitud>=spec_ceros-margen_spec_ceros && longitud<=spec_ceros+margen_spec_ceros) return 0;
	if (longitud>=spec_unos-margen_spec_unos && longitud<=spec_unos+margen_spec_unos) return 1;
	debug_printf (VERBOSE_DEBUG,"Invalid length for bit: %d",longitud);
	return -1;
}

int spec_lee_8_bits(void)
//Devuelve 8 bits leidos


//No usado:
//Se puede entrar la longitud anterior leida, si no entrarlo con -1

//Devuelve -1 si se llega al final del fichero
//Devuelve -2 si se encuentra ruido
//Devuelve -3 si se encuentran datos sin sentido
{
	unsigned char longitud,amplitud;
	char bit;
	int n,byte=0;

	for (n=0;n<8;n++) {

		if (spec_lee_onda(&longitud,&amplitud)==-1) return -1;

		if (amplitud<SPEC_NO_RUIDO) return -2;
		bit=spec_dice_bit(longitud);
		if (bit==-1) return -3;

		byte=byte*2+bit;
	}
	return byte;
}

void spec_debug_cabecera(int indice,int leidos)
//Escribe tipo de fichero
{
	int n;
	z80_int len,parm1,parm2;
	unsigned char tipo;

	char buffer_nombre[11];

	if (leidos!=19) {
		debug_printf (VERBOSE_INFO,"Read tape block. %s:%d . Length: %d",
			      spec_tipos_fichero[4],spec_smp_memory[indice],  ( leidos>2 ? leidos-2 : leidos  )  );

		//if (leidos>2) printf ("%u+2\n",leidos-2);
		//else printf ("%u\n",leidos);
		return;
	}

	tipo=spec_smp_memory[indice+1];

	for (n=0;n<10;n++) buffer_nombre[n]=spec_da_ascii(spec_smp_memory[indice+2+n]);
	buffer_nombre[10]=0;
	debug_printf (VERBOSE_INFO,"Read tape block. Standard Header - %s:%s",spec_tipos_fichero[tipo],buffer_nombre);

	len=value_8_to_16(spec_smp_memory[indice+13],spec_smp_memory[indice+12]);
	parm1=value_8_to_16(spec_smp_memory[indice+15],spec_smp_memory[indice+14]);
	parm2=value_8_to_16(spec_smp_memory[indice+17],spec_smp_memory[indice+16]);

	debug_printf (VERBOSE_INFO,"- Length:%u Parm1: %u Parm2: %u",len,parm1,parm2);


	int variables=len-parm2;
	if (variables<0) variables=0;

	if (tipo==3) {
		debug_printf (VERBOSE_INFO,"- Start:%u",parm1);
	}

	if (!tipo) {
		if (parm1<=32767) debug_printf (VERBOSE_INFO,"- Variables:%u . Autorun: %d",variables,parm1);
		else debug_printf (VERBOSE_INFO,"- Variables:%u . Autorun: None",variables);
	}


}

int main_spec_rwaatap(void)
{

	spec_smp_write_index_tap=0;
	spec_smp_read_index_tap=0;

	spec_smp_total_read=0;



	unsigned char amplitud,longitud;
	int byte,byte2;
	unsigned int n;


	//apunta al principio de cada bloque TAP
	int spec_smp_write_index_tap_start;

	debug_printf (VERBOSE_INFO,"Reading SMP audio data and converting to TAP file in memory");


	spec_smp_write_index_tap_start=spec_smp_write_index_tap;

	//dejamos espacio para los 2 bytes que indican longitud
	spec_smp_write_index_tap +=2;



	//Asignar memoria. 1 MB maximo
	if (spec_smp_memory==NULL) {
		debug_printf (VERBOSE_INFO,"Allocating %d bytes for tape buffer",MAX_BYTES_READ);

		spec_smp_memory=malloc(MAX_BYTES_READ);
		if (spec_smp_memory==NULL) {
			cpu_panic ("Error allocating memory for tape buffer");
		}
	}

	do {
		spec_carry=0;

		spec_cambio=0;
		spec_final_fichero=0;

		spec_bytes_leidos=0;

		//printf ("antes spec_smp_write_index_tap: %d spec_smp_write_index_tap_start: %d spec_smp_total_read: %d\n",spec_smp_write_index_tap,spec_smp_write_index_tap_start,spec_smp_total_read);



		//Leer unas ondas de tono guia
		n=0;
		do {
			if (spec_lee_onda(&longitud,&amplitud)==-1) {
				//printf ("spec_lee_onda == -1 antes de leer pilot tone. n=%d\n",n);
				goto fin;
			}
			if (amplitud<SPEC_NO_RUIDO || (!(longitud>=spec_tono_guia-margen_spec_tono_guia && longitud<=spec_tono_guia+margen_spec_tono_guia))
			) {
				//printf ("reset n. amplitud: %u longitud: %u\n",amplitud,longitud);
				n=0;
				continue;
			}
			n++;
			//printf ("ondas guia: %d\n",n);
		} while (n<SPEC_ONDAS_GUIA);

		debug_printf (VERBOSE_DEBUG,"Reading pilot tone...");
		do {
			if (spec_lee_onda(&longitud,&amplitud)==-1) goto fin;
		} while (amplitud>=SPEC_NO_RUIDO && (longitud>=spec_tono_guia-margen_spec_tono_guia &&
		longitud<=spec_tono_guia+margen_spec_tono_guia));

		//Hay que saber si se esta en mitad o al final de la onda falsa
		if (longitud>spec_mitad_onda_falsa) { //en mitad de la onda falsa
			spec_cambio=0;
			byte=spec_byte_cambio;
			do {
				byte2=spec_lee_byte();
				if (spec_final_fichero) goto fin;
			} while (spec_da_signo(byte)==spec_da_signo(byte2));
		}

		debug_printf (VERBOSE_DEBUG,"Reading data...");


		//Despues del tono guia viene una onda falsa, no utilizable,
		//parecida a un bit 0

		do {
			byte=spec_lee_8_bits(/*-1*/);
			//if (byte==-1) goto fin;
			if (byte==-1) break;
			if (byte<0) break;

			if (spec_smp_write_mem_byte(spec_smp_write_index_tap,byte)) {
				debug_printf (VERBOSE_ERR,"Memory buffer full");
				return 0;
			}

			spec_smp_write_index_tap++;


			spec_carry^=byte;
			spec_bytes_leidos++;
		} while (1);


		if (spec_bytes_leidos) {

			spec_debug_cabecera(spec_smp_write_index_tap_start+2,spec_bytes_leidos);

			n=spec_bytes_leidos;


		}

		if (spec_carry) {
			debug_printf (VERBOSE_ERR,"Loading Error. Invalid end carry");
		}


		if (spec_bytes_leidos) {

			debug_printf (VERBOSE_DEBUG,"Writing %d bytes to memory buffer",spec_bytes_leidos);

			spec_smp_write_mem_byte(spec_smp_write_index_tap_start,value_16_to_8l(spec_bytes_leidos));
			spec_smp_write_index_tap_start++;


			//comprobamos solo el ultimo byte, ya es suficiente
			if (spec_smp_write_mem_byte(spec_smp_write_index_tap_start,value_16_to_8h(spec_bytes_leidos) )) {
				debug_printf (VERBOSE_ERR,"Memory buffer full");
				return 0;
			}


			spec_smp_total_read+=spec_bytes_leidos+2;



			spec_smp_write_index_tap_start=spec_smp_write_index_tap;

			//dejamos espacio para los 2 bytes que indican longitud
			spec_smp_write_index_tap +=2;


		}
		else {
			debug_printf (VERBOSE_DEBUG,"0 bytes read");
		}
		//printf ("despues spec_smp_write_index_tap: %d spec_smp_write_index_tap_start: %d spec_smp_total_read: %d\n",spec_smp_write_index_tap,spec_smp_write_index_tap_start,spec_smp_total_read);


		debug_printf (VERBOSE_INFO,"----------------");


	} while (1);

	fin:

	fclose(ptr_mycinta_smp);

	if (spec_smp_total_read==0) {
		debug_printf(VERBOSE_INFO,"Converted Zero bytes of data from SMP file. May be a corrupted file or unsupported format");
	}




	return 0;

}
