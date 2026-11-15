/******************************************************************************
 * 
 * This software module was originally developed by 
 *
 *   Olaf Kaehler (Fraunhofer IIS)
 * 
 * and edited by 
 * 
 * 
 * 
 * in the course of development of the ISO/IEC 13818-3 for reference purposes
 * and its performance may not have been optimized. This software module is
 * an implementation of one or more tools as specified by the ISO/IEC 13818-3.
 * ISO/IEC gives You a royalty-free, worldwide, non-exclusive, copyright
 * license to copy, distribute, and make derivative works of this software
 * module ormodifications thereof for use in implementations of the 
 * ISO/IEC 13818-3  in products that satisfy conformance criteria (if any).
 * Those intending to use this software module in products are advised that 
 * its use may infringe existing patents. ISO/IEC have no liability for use of
 * this software module or modifications thereof.
 * Copyright is not released for products that do not conform to audiovisual
 * and image-coding related ITU Recommendations and/or ISO/IEC International
 * Standards. 
 * 
 ******************************************************************************/

/**********************************************************************
 * ISO MPEG Audio Subgroup Software Simulation Group (1996)
 * ISO 13818-3 MPEG-2 Audio Decoder - Lower Sampling Frequency Extension
 *
 * $Id: create_tables.c,v 1.2 2007/09/27 13:02:15 matejkml Exp $
 *
 * $Log: create_tables.c,v $
 * Revision 1.2  2007/09/27 13:02:15  matejkml
 * added copyright info
 *
 * Revision 1.1  2004/09/29 14:57:09  kaehleof
 * moved the decoder tables into tables.c and .h;
 * no more tables/ directory needed now;
 * create_tables.c was used to create tables.c and .h
 * from the files in the tables/ directory;
 *
 *
 **********************************************************************/
#include "huffman.h"
#include "common.h"

static const char* DEFAULT_TABLE_PATH="tables/";
static const int number_of_alloc_tables = 5;

static const char* table_path;

FILE *OpenTableFile(char *name)
{
  char fulname[80];

  FILE *f;

  fulname[0] = '\0';

  strcpy(fulname, table_path);
  strcat(fulname, name);

  if( (f=fopen(fulname,"r"))==NULL ) {
    fprintf(stderr,"OpenTable: could not find %s\n", fulname);
  }
  return f;
}


/* bit alloc tables: read them */

static int read_bit_alloc( int table, al_table *alloc ) /* read in table, return # subbands */
{
  unsigned int a, b, c, d, i, j;
  FILE *fp;
  char name[32], t[80];
  int sblim;
  
  strcpy(name, "alloc_0");
  
  switch (table) {
  case 0 : name[6] = '0';         break;
  case 1 : name[6] = '1';         break;
  case 2 : name[6] = '2';         break;
  case 3 : name[6] = '3';         break;
  case 4 : name[6] = '4';         break; /* LSF, added 1995-07-11 shn */
  default : name[6] = '0';
  }
  
  if (!(fp = OpenTableFile(name))) {
    printf("Please check bit allocation table %s\n", name);
    exit(1);
  }
  
  fgets(t, 80, fp);
  sscanf(t, "%d\n", &sblim);
  while (!feof(fp)) {
    fgets(t, 80, fp);
    sscanf(t, "%u %u %u %u %u %u\n", &i, &j, &a, &b, &c, &d);
    (*alloc)[i][j].steps = a;
    (*alloc)[i][j].bits  = b;
    (*alloc)[i][j].group = c;
    (*alloc)[i][j].quant = d;
  }
  fclose(fp);
  return sblim;
}

/* bit alloc tables: print them out */
void print_bit_alloc(FILE *out, al_table *t, int sb_lim)
{
  int i, j;
  for (i=0; i<sb_lim; i++) {
    if (i>0) fprintf(out, ",\n");
    fprintf(out, "  {\n");
    for (j=0; j<16; j++) {
      if (j>0) fprintf(out, ",\n");
      fprintf(out, "    {%i, %i, %i, %i}",
            (*t)[i][j].steps, (*t)[i][j].bits,
            (*t)[i][j].group, (*t)[i][j].quant);
    }
    fprintf(out, "\n  }");
  }
}


/* window: read the table */
void read_syn_window( double window[HAN_SIZE] )
{
  int i,j[4];
  FILE *fp;
  double f[4];
  char t[150];
  
  if (!(fp = OpenTableFile("dewindow") )) {
    printf("Please check synthesis window table 'dewindow'\n");
    exit(-1);
  }
  for (i=0;i<512;i+=4) {
    fgets(t, 150, fp);
    sscanf(t,"D[%d] = %lf D[%d] = %lf D[%d] = %lf D[%d] = %lf\n",
           j, f,j+1,f+1,j+2,f+2,j+3,f+3);
    if (i==j[0]) {
      window[i] = f[0];
      window[i+1] = f[1];
      window[i+2] = f[2];
      window[i+3] = f[3];
    } else {
      printf("Check index in synthesis window table\n");
      fclose(fp);
      exit(-1);
    }
    fgets(t,150,fp);
  }
  fclose(fp);
}

/* huffmann */
int read_huffmann(struct huffcodetab *ht)
{
  FILE *fi;
  int n;

  if (!(fi = OpenTableFile("huffdec") )) {
    fprintf(stderr,"Please check huffman table 'huffdec'\n");
    return(HUFFMAN_TABLE_ERROR);
  }

  /* from read_decoder_table() */
  {
    int i,nn,t;
    unsigned int v0,v1;
    char command[100],line[100];
    for (n=0;n<HTN;n++) {
      ht[n].val=NULL;
      /* .table number treelen xlen ylen linbits */ 
      do {
        fgets(line,99,fi);
      } while ((line[0] == '#') || (line[0] < ' '));
     
      sscanf(line,"%s %s %u %u %u %u",command,ht[n].tablename,
             &ht[n].treelen, &ht[n].xlen, &ht[n].ylen, &ht[n].linbits);
      if (strcmp(command,".end")==0)
        return n;
      else if (strcmp(command,".table")!=0) {
        fprintf(stderr,"huffman table %u data corrupted\n",n);
        return -1;
      }
      ht[n].linmax = (1<<ht[n].linbits)-1;
   
      sscanf(ht[n].tablename,"%d",&nn);
      if (nn != n) {
        fprintf(stderr,"wrong table number %u\n",n);
        return(-2);
      } 
      do {
        fgets(line,99,fi);
      } while ((line[0] == '#') || (line[0] < ' '));
      
      sscanf(line,"%s %d",command,&t);
      if (strcmp(command,".reference")==0) {
        ht[n].ref   = t;
        ht[n].val   = ht[t].val;
        ht[n].treelen  = ht[t].treelen;
        if ( (ht[n].xlen != ht[t].xlen) ||
             (ht[n].ylen != ht[t].ylen)  ) {
          fprintf(stderr,"wrong table %u reference\n",n);
          return (-3);
        };
        while ((line[0] == '#') || (line[0] < ' ') ) {
          fgets(line,99,fi);
        }
      }    
      else if (strcmp(command,".treedata")==0) {
        ht[n].ref  = -1;
        ht[n].val = (unsigned char (*)[2]) calloc(2*(ht[n].treelen),sizeof(unsigned char));
        if (ht[n].val == NULL) {
          fprintf(stderr, "heaperror at table %d\n",n);
          exit (-10);
        }
        for (i = 0; (unsigned int)i < ht[n].treelen; i++) {
          fscanf(fi,"%x %x",&v0, &v1);
          ht[n].val[i][0]=(unsigned char)v0;
          ht[n].val[i][1]=(unsigned char)v1;
        }
        fgets(line,99,fi); /* read the rest of the line */
      }
      else {
        fprintf(stderr,"huffman decodertable error at table %d\n",n);
      }
    }
  }

  fclose(fi);
  if (n!=HTN){
    fprintf(stderr,"decoder table read error\n");
  }
  return n;
}


void print_huffmann(FILE *c_file, FILE *h_file, struct huffcodetab *ht, int num)
{
  int i, j;
  for (i=0; i<num; i++) {
    if (ht[i].ref!=-1) continue;
    fprintf(c_file, "static unsigned char (hufftable%i[])[2] = {", i);
    if (ht[i].treelen==0) {
      fprintf(c_file,"{0,0}};\n");
      continue;
    }
    for (j=0; j<ht[i].treelen; j++) {
      if (j>0) fprintf(c_file, ", ");
      if ((j%6)==0) fprintf(c_file,"\n");
      fprintf(c_file, "{0x%02x,0x%02x}", ht[i].val[j][0], ht[i].val[j][1]);
    }
    fprintf(c_file, "};\n");
  }
  fprintf(c_file, "\n");

  fprintf(c_file,
          "/* array of all huffcodtable headers */\n"
          "/* 0..31 Huffman code table 0..31    */\n"
          "/* 32,33 count1-tables               */\n"
          "struct huffcodetab ht[HTN] = {\n");

  for (i=0; i<num; i++) {
    int tbl = i;
    if (i>0) fprintf(c_file, ",\n");
    if (ht[i].ref!=-1) tbl=ht[i].ref;
    fprintf(c_file, "  {\"%s\", %i, %i, %i, %i, %i, NULL, NULL, hufftable%i, %i}",
            ht[i].tablename, ht[i].xlen, ht[i].ylen, ht[i].linbits, ht[i].linmax, ht[i].ref,
            (ht[i].ref==-1)?i:ht[i].ref, ht[i].treelen);
  }
  fprintf(c_file, "\n};\n\n");

  fprintf(h_file, "extern struct huffcodetab ht[HTN];");
}
  


int main(int argc, char**argv)
{
  int i;
  FILE
    *c_file = stdout,
    *h_file = stderr;

  table_path = DEFAULT_TABLE_PATH;
  if (argc==2) {
    table_path = argv[1];
  }

  fprintf(c_file,
          "/* automatically created */\n"
          "#include \"tables.h\"\n\n");

  fprintf(h_file,
          "/* automatically created */\n\n"
          "#ifndef _TABLES_H_INCLUDED_\n"
          "#define _TABLES_H_INCLUDED_\n\n"
          "#include \"common.h\"\n"
          "#include \"huffman.h\"\n\n");
  

  /* alloc tables */
  {
    int sb_limits[number_of_alloc_tables];
    fprintf(c_file, "static const al_table alloc_tables[%i] = {\n",
            number_of_alloc_tables);
    for (i=0; i<number_of_alloc_tables; i++) {
      al_table tmp_table;
      int sb_lim;
      if (i>0) printf(",");
      fprintf(c_file, "{\n");
      memset(tmp_table, 0, sizeof(al_table));
      sb_lim = read_bit_alloc(i, &tmp_table);
      sb_limits[i] = sb_lim;
      print_bit_alloc(c_file, &tmp_table, sb_lim);
      fprintf(c_file, "\n}");
    }
    fprintf(c_file, "\n};\n\n");

    fprintf(c_file, "static const int sb_limits[%i] = {", number_of_alloc_tables);
    for (i=0; i<number_of_alloc_tables; i++) {
      if (i>0) printf(", ");
      fprintf(c_file, "%i", sb_limits[i]);
    }
    fprintf(c_file, "};\n\n");

    fprintf(c_file,
            "int get_bit_alloc(int table, al_table *alloc)\n"
            "{\n"
            "  if ((table>4)||(table<0)) table=0;\n"
            "  memcpy(alloc,&(alloc_tables[table]), sizeof(al_table));\n"
            "  return sb_limits[table];\n"
            "}\n\n");
    fprintf(h_file,
            "int get_bit_alloc(int table, al_table *alloc);\n");
  }

  /* window */
  {
    double window[HAN_SIZE];
    read_syn_window(window);
    fprintf(c_file, "static const double syn_window[HAN_SIZE] = {\n  ");
    for (i=0; i<HAN_SIZE; i++) {
      fprintf(c_file, "%12.9lf", window[i]);
      if (i==HAN_SIZE-1) break;
      if ((i%4)==3) fprintf(c_file,",\n  ");
      else fprintf(c_file,", ");
    }
    fprintf(c_file, "};\n\n");

    fprintf(c_file,
            "const double* get_syn_window()\n"
            "{\n"
            "  return syn_window;\n"
            "}\n\n");

    fprintf(h_file,
            "const double* get_syn_window();\n");
  }

  /* huffman */
  {
    int num;
    struct huffcodetab ht[HTN];
    num = read_huffmann(ht);
    print_huffmann(c_file, h_file, ht, num);
  }

  fprintf(h_file,"\n#endif\n");

  return 0;
}

