 /*

  constants are defined H (height), W (width), IMAGE_IN (name of the input file), IMAGE_OUT (output file)
  you have to change these values if you want to change the image to process

  In MPI form
  to obtain exec code:
  mpicc im_proc.c -o im_proc
  to execute:
  mpirun -np int ./im_proc
*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>


#define W 943
#define H 827
#define IMAGE_IN "logo.pgm"
#define IMAGE_OUT "mohameddd.pgm"
#define IMAGE_OUT2 "m.pgm"
/*  global variables */
int image_in[H][W];
int image_out[H][W];

/* function declarations */
void read_image (int image[H][W], char file_name[], int *p_h, int *p_w, int *p_levels);
void write_image (int image[H][W], char file_name[], int h, int w, int levels);
int ii=0;

int main(int argc, char *argv[])
{
  int i, j, h, w, levels ;
  struct timeval tdeb, tfin;

  // Read Image from file
  read_image (image_in, IMAGE_IN, &h, &w, &levels);

  //Filter
    int k[3][3]= {
  	  {-3, -1, -3},
  	  {0, 0, 0},
  	  {3, 1, 3}
  	};

  gettimeofday(&tdeb, NULL);
  //Transform image matrix to dynamic allocation

  int r = h;//number of rows, number of elements in vertical line 827
  int c = w;//number of columns, number of elements in hori line 943
  int *image_in_malloc = malloc(r*c*sizeof(int));
    for (i = 0; i < h; i++) {
      for (j = 0; j < w; j++) {
        image_in_malloc[i*c+j] = image_in[i][j];
      }
    }
  // Start Paralell Processing

  //Start MPI  
  int rank,size;
int *a, *b;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);


  a = malloc(sizeof(int)*r*c/size);
  
  b = malloc(sizeof(int)*r*c);
  int *image_out_malloc = malloc(r*c*sizeof(int));
    
  //Divide up data  
  int *sendcounts;
  sendcounts = malloc(sizeof(int)*size);
  int *displs;
  displs = malloc(sizeof(int)*size);
  //int rec_buf[779861];

  int rem = (H*W)%size; // elements remaining after division among processes
  int sum = 0;
  
 // calculate send counts and displacements
  for (i = 0; i < size; i++) {
    sendcounts[i] = (H*W)/size;
    if (rem > 0) {
      sendcounts[i]++;
      rem--;
    }
    displs[i] = sum;
    sum += sendcounts[i];
  }

  // print calculated send counts and displacements for each process
 
  if (0 == rank) {
    for (i = 0; i < size; i++) {
      printf("sendcounts[%d] = %d\tdispls[%d] = %d\n", i, sendcounts[i], i, displs[i]);
      printf("sendcounts[%d] = %d\n", i, sendcounts[i]);
      
    }
  }

  //Scatter work to each processor

  
  int send_count =(H/size)*W; 
 
     printf(" Scatterv -> task : %d : master send : sendcounts[%d]= %d ,  displs[%d]=%d ,  send_count = %d \n", rank,rank,sendcounts[rank],rank,displs[rank],send_count);
    MPI_Scatterv(image_in_malloc, sendcounts,displs, MPI_INT, a, sendcounts[rank], MPI_INT,0, MPI_COMM_WORLD);
    gettimeofday(&tdeb, NULL);
    
    //Each Processor applies fiter to image
    
    for (i = 1; i < r/size-1; i++) {
      for (j = 1; j < c; j++) {
        if (i == 0 || i == c-1){ 
          image_out_malloc[c*i+j] = a[c*i+j];
        }
        else if (j == 0 || j == c-1) {
          image_out_malloc[c*i+j] = a[c*i+j];
        } 
        else {
          image_out_malloc[c*i+j] = a
          [c*(i-1)+(j-1)]*k[0][0]+a
          [c*(i-1)+(j)]*k[0][1]+a
          [c*(i-1)+(j+1)]*k[0][2] +a
          [c*(i)+(j-1)]*k[1][0]+a
          [c*(i)+(j)]*k[1][1]+a
          [c*(i)+(j+1)]*k[1][2] +a
          [c*(i+1)+(j-1)]*k[2][0]+a
          [c*(i+1)+(j)]*k[2][1]+a
          [c*(i+1)+(j+1)]*k[2][2];
        }
        if (image_out_malloc[c*i+j] > 255) {
          image_out_malloc[c*i+j] = 255;
        } 
        else if (image_out_malloc[c*i+j] < 0) {
          image_out_malloc[c*i+j] = 0;
        }
      }
    }

    gettimeofday(&tfin, NULL);

  // Gather all the information back to one processor

  int recvcounts=779861/size;
   printf(" Gatherv -> task : %d : master receive : sendcounts[%d] = %d  , recvcounts = %d \n", rank,rank,sendcounts[rank],recvcounts);
  MPI_Gatherv(image_out_malloc, sendcounts[rank], MPI_INT, b,sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

  // Turn malloc back into matrix



  if(rank==0){
   
  for (i = 0; i < r; i++) {
    for (j = 0; j < c; j++) {
      int r=(i*c + j);
      image_out[i][j] = b[r];
    }
  }
  
  //Write image  to file
  write_image (image_out, IMAGE_OUT, h, w, levels);
  }

  printf ("computation time (microseconds): %ld\n",  (tfin.tv_sec - tdeb.tv_sec)*1000000 + (tfin.tv_usec - tdeb.tv_usec));
  // End MPI

  MPI_Finalize();

  // Free up mallocs
  free(sendcounts);
  free(displs);
  free(image_in_malloc);
  free(image_out_malloc);
  free(a);
  free(b);

return 0;
}

void read_image (int image[H][W], char file_name[], int *p_h, int *p_w, int *p_levels){
  FILE *fin;
  int i, j, h, w, levels ;
  char buffer[80];

  /* open P2 image */
  fin = fopen (IMAGE_IN, "r");
  if (fin == NULL){
    printf ("file open error\n");
    exit(-1);
  }
  fscanf (fin, "%s", buffer);
  if (strcmp (buffer, "P2")){
    printf ("the image format must be P2\n");
    exit(-1);
  }
  fgets (buffer, 80, fin);
  fgets (buffer, 80, fin);
  fscanf (fin, "%d%d", &w, &h);
  fscanf (fin, "%d", &levels);
  printf ("image reading ... h = %d w = %d\n", h, w);

  /* pixels reading */
  for (i = 0; i < h ; i++)
    for (j = 0; j < w; j++)
       fscanf (fin, "%d", &image[i][j]);
  fclose (fin);

  *p_h = h;
  *p_w = w;
  *p_levels = levels;
  return;

}
void write_image (int image[H][W], char file_name[], int h, int w, int levels){
  FILE *fout;
  int i, j;

  /* open the file */
  fout=fopen(IMAGE_OUT,"w");
  if (fout == NULL){
    printf ("file opening error\n");
    exit(-1);
  }

  /* header write */
  fprintf(fout,"P2\n# test \n%d %d\n%d\n", w, h, levels);
  /* format P2, commentary, w x h points, levels */

  /* pixels writing*/
  for (i = 0; i < h ; i++)
    for (j = 0; j < w; j++)
       fprintf (fout, "%d\n", image[i][j]);

  fclose (fout);
  return;
}


 


    // print what each process receivedclea
    /*
    printf("%d: ", rank);
    for (int i = 0; i < sendcounts[rank]; i++) {
        printf("%c\t", rec_buf[i]);
    }""
    printf("\n");
    */
