/*Author: Lee Carraher
#Institution: University of Cincinnati, Computer Science Dept.


# this is a nearest lattice point decoder based on the hexacode based decoder of
#Amrani, Be'ery IEEE Trans. on Comm. '96, with initial construction
#from  Amrani, Be'ery,Vardy, Sun,Tilborg IEEE Info Thry'94

# the goal is to rewrite this algorithm in efficient C for cuda
# and eventual use as a Hashing Function
# for use in a Cuda Parallel Locality Hash Based Clustering algorithm
# additional implementation may include MPI/Cuda, and
#anonymous offline data clustering


#-------------QAM Stuff ----------------------
# use a curtailed QAM for all positive signals
#  7 A000 B000 A110 B110
#  5 B101 A010 B010 A101
#  3 A111 B111 A001 B001
#  1 B011 A100 B100 A011
#  0   1         3       5      7
# still gets rotated    \ 4 /
#                               1 \/ 3
#                         /\
#                           / 2 \

Bs           100
55    55    51  55   77   37   77   77    33  77   33   73
010 010 001 010 011 000 011 011 111 011 111 100
7.0,3.0,   3.0,3.0,   7.0,7.0   ,   3.0,3.0,   7.0,7.0,    7.0,7.0,    3.0,7.0,   7.0,7.0,    5.0,5.0   ,    5.0,1.0,   5.0,5.0,   5.0,5.0



# leech decoder uses a rotated Z2 lattice, so to find leading cosets
# just find the nearest point in 64QAM, A,B ; odd, even| to the rotated
# input vector
# rotate using the standard 2d rotation transform
#                      [cos x -sin x ]
#                  R = [sin x  cos x ]    cos(pi/4) = sin(pi/4)=1/sqrt(2)
# for faster C implementation use these binary fp constants
# 1/sqrt(2) = cc3b7f669ea0e63f ieee fp little endian
#           = 3fe6a09e667f3bcc ieee fp big endian
#           = 0.7071067811865475244008
#
#v' = v * R
# integer lattice
#
#  4 A000 B000 A110 B110 | A000 B000 A110 B110
#  3 B101 A010 B010 A101 | B101 A010 B010 A101
#  2 A111 B111 A001 B001 | A111 B111 A001 B001
#  1 B011 A100 B100 A011 | B011 A100 B100 A011
#    --------------------|---------------------
# -1 A000 B000 A110 B110 | A000 B000 A110 B110
# -2 B101 A010 B010 A101 | B101 A010 B010 A101
# -3 A111 B111v A001 B001 | A111 B111 A001 B001
# -4 B011 A100 B100 A011 | B011 A100 B100 A011
#even pts {000,110,111,001}
#odd  pts {010,101,100,011}
*/


/*
 * this thing converges really quickly this is more than enough for fp

inline float quicksqrt(float b)
{
    float x = 1.1;
    unsigned char i =0;

    for(;i<16;i++){
        x = (x+(b/x))/2.0;
    }

    return x;
}
*/
/*WARNING: not true euclidean distance
 * compute the distance between two 24 dimensional vectors.
 * The square-root is omitted because the algorithm only needs
 * to know which is closer d(cp, pt.) or d(cp',pt) , for which
 * sqrt(d(cp, pt.)) and sqrt(d(cp', pt.)) inequality holds for positive
 * distances(this is why we keep the squares).
 */
//#define golay

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inline float distance(
    float *cp,
    float pt[2])
{
 // printf("%f,%f : %f,%f = ",cp[0],cp[1],pt[0],pt[1]);
    float s =(cp[0]-pt[0])*(cp[0]-pt[0]) + (cp[1]-pt[1])*(cp[1]-pt[1]);
   // printf(" %f\n",s);
    return s;

}





/*
 * an integer symbol encoding of an H6 encoder.
 * 0 1 2 3 = 0 1 w w'
 * indexes of the array result in the H6 encoding
 * of a 3 integer symbol character equivalent word.
 * eg: H6CodeWords[0][1][2] = [0,3,2]
 *resulting in the codeword : 0 1 w 0 w' w
 */
static const unsigned char H6CodeWords[4][4][4][3]  = {
    {//0    0       1       w      -w
        {{0,0,0},{1,1,1},{2,2,2},{3,3,3}},//0
        {{1,2,3},{0,3,2},{3,0,1},{2,1,0}},//1
        {{2,3,1},{3,2,0},{0,1,3},{1,0,2}},//w
        {{3,1,2},{2,0,3},{1,3,0},{0,2,1}}},//-w
    {//1    0       1       w      -w
        {{1,3,2},{0,2,3},{3,1,0},{2,0,1}},//0
        {{0,1,1},{1,0,0},{2,3,3},{3,2,2}},//1
        {{3,0,3},{2,1,2},{1,2,1},{0,3,0}},//w
        {{2,2,0},{3,3,1},{0,0,2},{1,1,3}}//-w
    },
    {//w    0       1       w      -w
        {{2,1,3},{3,0,2},{0,3,1},{1,2,0}},//0
        {{3,3,0},{2,2,1},{1,1,2},{0,0,3}},//1
        {{0,2,2},{1,3,3},{2,0,0},{3,1,1}},//w
        {{1,0,1},{0,1,0},{3,2,3},{2,3,2}}//-w
    },
    {//-w   0       1       w      -w
        {{3,2,1},{2,3,0},{1,0,3},{0,1,2}},//0
        {{2,0,2},{3,1,3},{0,2,0},{1,3,1}},//1
        {{1,1,0},{0,0,1},{3,3,2},{2,2,3}},//w
        {{0,3,3},{1,2,2},{2,1,1},{3,0,0}}//-w
    }
};


static const unsigned char H6CodeWordsRev[4][4][4][3]  = {
    {
    //#w    0       1       w      -w
    { {0,0,0}, {3,2,1}, {1,3,2}, {2,1,3} }, //0
    { {2,3,1}, {1,1,0}, {3,0,3}, {0,2,2} },//1
    { {3,1,2}, {0,3,3}, {2,2,0}, {1,0,1} }, //w
    { {1,2,3}, {2,0,2}, {0,1,1}, {3,3,0} }//-w
    },
    {//w    0       1       w      -w
    { {1,1,1}, {2,3,0}, {0,2,3}, {3,0,2} },//0
    { {3,2,0}, {0,0,1}, {2,1,2}, {1,3,3} },//1
    { {2,0,3}, {1,2,2}, {3,3,1}, {0,1,0} },//w
    { {0,3,2}, {3,1,3}, {1,0,0}, {2,2,1} }//-w
    },
    {//w    0       1       w      -w
    { {2,2,2}, {1,0,3}, {3,1,0}, {0,3,1} },//0
    { {0,1,3}, {3,3,2}, {1,2,1}, {2,0,0} },//1
    { {1,3,0}, {2,1,1}, {0,0,2}, {3,2,3} },//w
    { {3,0,1}, {0,2,0}, {2,3,3}, {1,1,2} }//-w
    },
    {//-w   0       1       w      -w
    { {3,3,3}, {0,1,2}, {2,0,1}, {1,2,0} },//0
    { {1,0,2}, {2,2,3}, {0,3,0}, {3,1,1} },//1
    { {0,2,1}, {3,0,0}, {1,1,3}, {2,3,2} },//w
    { {2,1,0}, {1,3,1}, {3,2,2}, {0,0,3} }//-w
    }};


/*
#define APT  1
#define BPT  3
#define CPT  5
#define DPT  7
*/

#define APT  -.75
#define BPT  -.25
#define CPT  .25
#define DPT  .75

// shaping -.75, -.25,+.25,+.75
//the unit scaled points of 16QAM centered at the origin.
// along with their golay code + parity bit representations
//000, 110 , 001, 111
float evenAPts[4][2] = {{APT, DPT},{CPT, DPT},{CPT, BPT},{APT, BPT}};
//010 100 011 101
float oddAPts[4][2]  ={{BPT, CPT},{BPT, APT},{DPT, APT},{DPT, CPT}};
//000, 110 , 001, 111
float evenBPts[4][2] = {{BPT, DPT},{DPT, DPT},{DPT, BPT},{BPT, BPT}};
//010 100 011 101
float oddBPts[4][2]  = {{CPT, CPT},{CPT, APT},{APT, APT},{APT, CPT}};



void pp(
    unsigned long ret,
    int ct,
    int grsize)
{
  int i,j;//,err;
  for(i=0;i<ct;i++)
  {
      for(j=0;j<grsize;j++)
      {
          printf("%li",ret&1);
          //err +=ret&1;
          ret=ret>>1;

      }
      printf(" ");
  }
  //if(err%2) printf("error \n");else
    printf("\n");
}

void convertToCoords(
    unsigned long long c,
    float* point)
{

  float axCoords[] = {APT,CPT, BPT,DPT,BPT,DPT,CPT,APT };
  float ayCoords[] = {DPT,BPT,CPT,APT,APT,CPT,DPT,BPT};
  float bxCoords[] = {BPT,DPT,CPT,APT,CPT,APT,DPT,BPT};
  float byCoords[] = {DPT,BPT, CPT,APT,APT,CPT,DPT,BPT};


  int parity = (c&0xfff);//seperate these parts



  //compute A/B point from parity
  int u = parity;
  int Bpoint = 0;
  while(u>0)
   {
      if((u &1)== 1)Bpoint++;
      u = (u>>1);
  }


  c=(c&0xffffff000)>>12;

  int i;
  int pt = 0;
  if((Bpoint &1))
    {
      for(i=0;i<12;i++){
                  pt = ((c&1)<<2)+(c&2)+(parity&1);
                  point[i*2]= bxCoords[pt];
                  point[i*2+1]=byCoords[pt];
                  c = c>>2;
                  parity = parity>>1;
            }
  }
  else{
      for(i=0;i<12;i++){
                pt = ((c&1)<<2)+(c&2)+(parity&1)  ;
                  point[i*2]= axCoords[pt];
                  point[i*2+1]=ayCoords[pt];
                  c = c>>2;
                  parity = parity>>1;
            }
  }


}




/*
*    this function returns all of the pertinent information
*    from the decoder such as minimum distances, nearest
*    coset leader quadrant, and alternative k-parity distances
*
* these maps are separated into the quadrants of a Cartesian
*  plane now we gotta order these properly
*
* another simple fix is that the quadrants of QAM be abstractly
* defined, and the -,+ of order pairs be used to tile the
* generalized 16bit qam, besides this has to be done anyway
*  so we can get out the real number coordinates in the end
 */
void QAM(
    float *r,
    float evenPts[4][2],
    float oddPts[4][2],
    float dijs[12][4],
    float dijks[12][4],
    unsigned char kparities[12][4])
{
//void QAM(float *r, float *evenPts,float *oddPts,float *dijs,float *dijks,int *kparities){


    //the closest even-type Z2 lattice point is used as the
    //coset representatives for all points, not currently used
    //quadrant = [0 for k in range(12)]

    unsigned char i = 0;

    for(;i<12;i++){

        float dist000 = distance(&r[i*2],evenPts[0]);
        float dist110 = distance(&r[i*2],evenPts[1]);
        float dist001 = distance(&r[i*2],evenPts[2]);
        float dist111 = distance(&r[i*2],evenPts[3]);

        if(dist000<dist001)
        {
             dijs[i][0]=dist000;
             dijks[i][0]=dist001;
             kparities[i][0] = 0;
        }
        else{
             dijs[i][0]=dist001;
             dijks[i][0]=dist000;
             kparities[i][0] = 1;
        }
        if(dist110<dist111){
             dijs[i][3]=dist110;
             dijks[i][3]=dist111;
             kparities[i][3] = 0;
        }
        else{
             dijs[i][3]=dist111;
             dijks[i][3]=dist110;
             kparities[i][3] = 1;
        }
        //quadrant[i] = 0


        //min over odds
        float dist010 = distance(&r[i*2],oddPts[0]);
        float dist100 = distance(&r[i*2],oddPts[1]);
        float dist011 = distance(&r[i*2],oddPts[2]);
        float dist101 = distance(&r[i*2],oddPts[3]);
        if (dist010<dist011){
             dijs[i][1]=dist010;
             dijks[i][1]=dist011;
             kparities[i][1] = 0;
        }
        else{
             dijs[i][1]=dist011;
             dijks[i][1]=dist010;
             kparities[i][1] = 1;
        }
        if (dist100<dist101){
             dijs[i][2]=dist100;
             dijks[i][2]=dist101;
             kparities[i][2] = 0;
        }
        else{
             dijs[i][2]=dist101;
             dijks[i][2]=dist100;
             kparities[i][2] = 1;
        }
    }
}



/*
    computes the Z2 block confidence of the concatenated points projections onto GF4 characters
*/
void blockConf(
    float dijs[12][4],
    float muEs[6][4],
    float muOs[6][4],
    unsigned char prefRepE[6][4][4],
    unsigned char prefRepO[6][4][4])
{


    //each two symbols is taken as a single character in GF4
    unsigned char i=0;
    for(; i<6;i++){

        //0000 1111
        float s = dijs[2*i][0]+dijs[2*i+1][0];
        float t = dijs[2*i][3]+dijs[2*i+1][3];
        if(s<t){
            muEs[i][0] = s;
            prefRepE[i][0][0] = 0;
            prefRepE[i][0][1] = 0;
            prefRepE[i][0][2] = 0;
            prefRepE[i][0][3] = 0;

        }
        else{
            muEs[i][0] = t;
            //prefRepE[i][0] = 15;//[1,1,1,1]
            prefRepE[i][0][0] = 1;
            prefRepE[i][0][1] = 1;
            prefRepE[i][0][2] = 1;
            prefRepE[i][0][3] = 1;
        }

        //0011 1100 0 3 3 0
        s = dijs[2*i][0]+dijs[2*i+1][3];
        t = dijs[2*i][3]+dijs[2*i+1][0];
        if(s<t){
            muEs[i][1] = s;
            //prefRepE[i][1] = 3;//[0,0,1,1]
            prefRepE[i][1][0] = 0;
            prefRepE[i][1][1] = 0;
            prefRepE[i][1][2] = 1;
            prefRepE[i][1][3] = 1;
        }
        else{
            muEs[i][1] = t;
            //prefRepE[i][1] = 12;//[1,1,0,0]
            prefRepE[i][1][0] = 1;
            prefRepE[i][1][1] = 1;
            prefRepE[i][1][2] = 0;
            prefRepE[i][1][3] = 0;
        }


        //1010 0101
        s = dijs[2*i][2]+dijs[2*i+1][2];
        t = dijs[2*i][1]+dijs[2*i+1][1];
        if (s<t){
            muEs[i][2] = s;
            //prefRepE[i][2] = 10;//[1,0,1,0]
            prefRepE[i][2][0] = 1;
            prefRepE[i][2][1] = 0;
            prefRepE[i][2][2] = 1;
            prefRepE[i][2][3] = 0;
            }
        else{
            muEs[i][2] = t;
            //prefRepE[i][2] = 5;//[0,1,0,1]
            prefRepE[i][2][0] = 0;
            prefRepE[i][2][1] = 1;
            prefRepE[i][2][2] = 0;
            prefRepE[i][2][3] = 1;
        }
        //0110 1001
        s = dijs[2*i][1]+dijs[2*i+1][2];
        t = dijs[2*i][2]+dijs[2*i+1][1];
        if(s<t){
            muEs[i][3] = s;
            //prefRepE[i][3] =6;// [0,1,1,0]
            prefRepE[i][3][0] = 0;
            prefRepE[i][3][1] = 1;
            prefRepE[i][3][2] = 1;
            prefRepE[i][3][3] = 0;
        }
        else{
            muEs[i][3] = t;
            //prefRepE[i][3] = 9;//[1,0,0,1]
            prefRepE[i][3][0] = 1;
            prefRepE[i][3][1] = 0;
            prefRepE[i][3][2] = 0;
            prefRepE[i][3][3] = 1;
        }



    //this operation could be parallel, but probably doesnt need to be

        //1000 0111
        s = dijs[2*i][2]+dijs[2*i+1][0];
        t = dijs[2*i][1]+dijs[2*i+1][3];
        if(s<t){
            muOs[i][0] = s;
            //prefRepO[i][0] = 8;//[1,0,0,0]
            prefRepO[i][0][0] = 1;
            prefRepO[i][0][1] = 0;
            prefRepO[i][0][2] = 0;
            prefRepO[i][0][3] = 0;
        }
        else{
            muOs[i][0] = t;
            //prefRepO[i][0] = 7;//[0,1,1,1]
            prefRepO[i][0][0] = 0;
            prefRepO[i][0][1] = 1;
            prefRepO[i][0][2] = 1;
            prefRepO[i][0][3] = 1;
        }

        //0100 1011
        s = dijs[2*i][1]+dijs[2*i+1][0];
        t = dijs[2*i][2]+dijs[2*i+1][3];
        if (s<t){
            muOs[i][1] = s;
            //prefRepO[i][1] = 4;//[0,1,0,0]
            prefRepO[i][1][0] = 0;
            prefRepO[i][1][1] = 1;
            prefRepO[i][1][2] = 0;
            prefRepO[i][1][3] = 0;
        }
        else{
            muOs[i][1] = t;
            //prefRepO[i][1] = 11;//[1,0,1,1]
            prefRepO[i][1][0] = 1;
            prefRepO[i][1][1] = 0;
            prefRepO[i][1][2] = 1;
            prefRepO[i][1][3] = 1;
        }

        //0010 1101
        s = dijs[2*i][0]+dijs[2*i+1][2];
        t = dijs[2*i][3]+dijs[2*i+1][1];
        if(s<t){
            muOs[i][2] = s;
            //prefRepO[i][2] =2;// [0,0,1,0]
            prefRepO[i][2][0] = 0;
            prefRepO[i][2][1] = 0;
            prefRepO[i][2][2] = 1;
            prefRepO[i][2][3] = 0;
        }
        else{
            muOs[i][2] = t;
            //prefRepO[i][2] = 13;//[1,1,0,1]
            prefRepO[i][2][0] = 1;
            prefRepO[i][2][1] = 1;
            prefRepO[i][2][2] = 0;
            prefRepO[i][2][3] = 1;
        }

        //0001 1110
        s = dijs[2*i][0]+dijs[2*i+1][1];
        t = dijs[2*i][3]+dijs[2*i+1][2];
        if(s<t){
            muOs[i][3] = s;
           // prefRepO[i][3] = 1;//[0,0,0,1]
            prefRepO[i][3][0] = 0;
            prefRepO[i][3][1] = 0;
            prefRepO[i][3][2] = 0;
            prefRepO[i][3][3] = 1;
        }
        else{
            muOs[i][3] = t;
            //prefRepO[i][3] = 14;//[1,1,1,0]
            prefRepO[i][3][0] = 1;
            prefRepO[i][3][1] = 1;
            prefRepO[i][3][2] = 1;
            prefRepO[i][3][3] = 0;
        }
    }

}


/*here we are looking for the least character in the H6 hexacode word
   returns the hexacode word and the wt, for using in locating the least reliable symbol
*/
void constructHexWord(
    float mus[6][4],
    unsigned char chars[6],
    float charwts[6])
{

    unsigned char i = 0;
    for(;i<6;i++)
    {
        unsigned char leastChar = 0;
        float leastwt = mus[i][0];

        if(mus[i][1]<leastwt){
            leastwt = mus[i][1];
            leastChar = 1;
        }

        if(mus[i][2]<leastwt){
            leastwt = mus[i][2];
            leastChar = 2;
        }

        if(mus[i][3]<leastwt){
            leastwt = mus[i][3];
            leastChar = 3;
        }

        chars[i] = leastChar;
        charwts[i]=leastwt;
    }
}

/*
    this is the minimization over the hexacode function using the 2nd algorithm of  amrani and be'ery ieee may '96
*/
float minH6(
    unsigned char  y[6],
    float charwts[6],
    float mus[6][4])
{






    //locate least reliable
    float leastreliablewt = charwts[0];
    unsigned char leastreliablechar = 0;
    if(charwts[1]>leastreliablewt){
        leastreliablewt = charwts[1];
        leastreliablechar = 1;
    }
    if(charwts[2]>leastreliablewt){
        leastreliablewt = charwts[2];
        leastreliablechar = 2;
    }


    //minimize over the 8 candidate Hexacode words
    float minCodeWt = 1000.0;
    unsigned char j = 0;
    //unsigned char  min = 0;
    float m_dist;

    unsigned char  leastcan[6] = {0,0,0,0,0,0};
    //build candidate list
   // unsigned char  candslst[8][6]=  {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},
   //                                                     {0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}};


    unsigned char  cand[6] = {0,0,0,0,0,0};


    //(unsigned char[8][6]) (malloc(8*6*sizeof(unsigned char)));//{
    unsigned char i;
    for(i = 0;i<4;i++){
        y[leastreliablechar] = i;
        cand[0] = y[0];
        cand[1] = y[1];
        cand[2] = y[2];
        cand[3] = H6CodeWords[y[0]][y[1]][y[2]][0];
        cand[4] = H6CodeWords[y[0]][y[1]][y[2]][1];
        cand[5] = H6CodeWords[y[0]][y[1]][y[2]][2];

        m_dist = 0.0;
        for( j=0;j<6;j++)m_dist += mus[j][cand[j]];
        if(m_dist < minCodeWt){
            minCodeWt = m_dist;
            for(j=0;j<6;j++)  leastcan[j] = cand[j];
        }
    }



    //y2
    //locate the least reliable symbol in each
    leastreliablewt = charwts[3];
    leastreliablechar = 3;
    if(charwts[4]>leastreliablewt){
        leastreliablewt = charwts[4];
        leastreliablechar = 4;
    }
    if(charwts[5]>leastreliablewt){
        leastreliablewt = charwts[5];
        leastreliablechar = 5;
    }

    for(;i<8;i++){
        y[leastreliablechar] = i-4;

        cand[0] = H6CodeWordsRev[y[3]][y[4]][y[5]][0];
        cand[1] = H6CodeWordsRev[y[3]][y[4]][y[5]][1];
        cand[2] = H6CodeWordsRev[y[3]][y[4]][y[5]][2];
        cand[3] = y[3] ;
        cand[4] = y[4];
        cand[5] = y[5] ;


        m_dist = 0.0;
        for( j=0;j<6;j++)m_dist += mus[j][cand[j]];

        if(m_dist < minCodeWt)
        {
            minCodeWt = m_dist;
            for(j=0;j<6;j++) leastcan[j] = cand[j];

        }
    }

    for(j=0;j<6;j++)y[j] = leastcan[j];

    //printf("%i %i %i   %i %i %i\n",y[0],y[1],y[2],y[3],y[4],y[5],y[6]);

    return minCodeWt;
}



/*
    here we are resolving the h-parity. which requires that the overall least significant bit parities equal the
    bit parities of each projected GF4 block. aka column parity must equal 1st row parity
*/
float hparity(
    float weight,
    unsigned char hexword[6],
    unsigned char prefReps[6][4][4],
    float dijs[12][4],
    unsigned char oddFlag,
    unsigned char * codeword)
{

   unsigned char parity= 0;
   unsigned char i;;



    for(i=0;i<6;i++){

        //create the golay codeword from the hexacode representation
        codeword[i*4]=prefReps[i][hexword[i]][0];
        codeword[i*4+1]=prefReps[i][hexword[i]][1];
        codeword[i*4+2]=prefReps[i][hexword[i]][2];
        codeword[i*4+3]=prefReps[i][hexword[i]][3];

        //
        parity = parity ^ prefReps[i][hexword[i]][0];//this should be the highest order bit

        //printf("%i%i%i%i ", codeword[i*4],codeword[i*4+1],codeword[i*4+2],codeword[i*4+3]);
    }

    if((parity&1) == oddFlag){
        //printf("\n");
        return weight;
    }
/*
    if(oddFlag)
    printf(" ---(Odd)>> ");
    else
      printf(" ---(Eve)>> ");
*/

    float leastwt = 1000.0;
    unsigned char least = 0;
    float deltaX;
    unsigned char idx1,idx2;
    //walk along the codeword again
    for(i=0;i<6;i++){

        idx1 =(codeword[4*i]<<1) +codeword[4*i+1];
        idx2 =(codeword[4*i+2]<<1) +codeword[4*i+3];
        // compute cost of complementing the hexacode representation ^3 of bits
        //select minimal cost complement.
        deltaX = (dijs[2*i][idx1^3] + dijs[2*i+1][idx2^3]) - (dijs[2*i][idx1] + dijs[2*i+1][idx2]);
        //printf("%f \n" , deltaX);

        if (deltaX < leastwt){
            leastwt = deltaX;
            least = i*4;
        }

    }


    weight = weight + leastwt;

    codeword[least]= codeword[least]^1;
    codeword[least+1]= codeword[least+1]^1;
    codeword[least+2]= codeword[least+2]^1;
    codeword[least+3]= codeword[least+3]^1;

    /*
    for(i=0;i<6;i++){
        printf("%i%i%i%i ", codeword[i*4],codeword[i*4+1],codeword[i*4+2],codeword[i*4+3]);
    }
    printf(": [%i , %f]\n" , least,leastwt);
  */
    return weight;
}

float kparity(
    float weight,
    unsigned char * codeword,
    unsigned char Btype,
    unsigned char * codeParity,
    float dijks[12][4],
    float dijs[12][4],
    unsigned char kparities[12][4])
{
    /*
        this last parity check assures that all A or B points have even/odd parity
    */
    unsigned char parity = 0;
    unsigned char i =0;

    float least =1000;
    float dif;
    unsigned char argLeast = 0;


    for( ;i <12;i++)
     {
        unsigned char n =(codeword[2*i]<<1)+codeword[2*i+1];
         parity= parity^kparities[i][n];
         codeParity[i] = kparities[i][n];

          dif = dijks[i][n]-dijs[i][n];
          if(dif <= least)
            {
              least = dif;
              argLeast = i;
          }
     }

/*something here as this parity check doesnt fix anything*/
//not sure why this doesnt at least double the set cardinality
    if(parity== Btype ){
        return weight;
    }

    codeParity[argLeast ]=  codeParity[argLeast ] ^1;

    return weight+least;
}


//unsigned char* decode(float r[12][2], float *distance){
//unsigned long long decodeLeech(float *r,float *distance)
unsigned long long decodeLeech(float *r,float *distance)
{

// #####################QAM Dijks ###################
    //float* dijs = malloc(sizeof(float)*12*4) ;
	float dijs[12][4];
    //float* dijks =malloc(sizeof(float)*12*4) ;
	float dijks[12][4];
    //there is a set for each quarter decoder, and the A/B_ij odd/even
    //unsigned char* kparities =malloc(sizeof(unsigned char)*12*4) ;
	unsigned char kparities[12][4];
    QAM(r,evenAPts,oddAPts,dijs,dijks,kparities);


    // #####################Block Confidences ###################
    //         0  1    w   W
    //float * muEs = malloc(sizeof(float)*6*4*4) ;
    float muEs[6][4];
    //float * muOs = malloc(sizeof(float)*6*4*4) ;
    float muOs[6][4];
    //unsigned char* prefRepE=malloc(sizeof(unsigned char)*6*4*4) ;
    unsigned char prefRepE[6][4][4];
    //unsigned char* prefRepO=malloc(sizeof(unsigned char)*6*4*4) ;
    unsigned char prefRepO[6][4][4];

    blockConf(dijs,muEs,muOs,prefRepE,prefRepO); //just run through both as its faster, but could conserve array allocation

    unsigned char i;

    // #####################Construct Hexacode Word ###################
    //unsigned char *y = malloc(sizeof(unsigned char)*6) ;
    unsigned char y[6];

    //float* charwts = malloc(sizeof(float)*6) ;
    float charwts[6];
    constructHexWord(muEs,y,charwts);


    // #####################Minimize over the Hexacode ###################
    //unsigned char* hexword =  malloc(sizeof(unsigned char)*6) ;
    float weight = minH6(y,charwts,muEs);



    //****chars = y = hexword *****
    //unsigned char* codeword =  malloc(sizeof(unsigned char)*24);
    unsigned char codeword[24];
    //unsigned char* codeParity =  malloc(sizeof(unsigned char)*12) ;
    unsigned char codeParity[12];

    int winner = 0;

    weight = hparity(weight,y,prefRepE,dijs,0,codeword);//byref
//    printf("\t");int d;
//    for(d = 0;d<6;d++)printf("%i ", y[d]);
    weight =kparity(weight,codeword,0, codeParity,dijks,dijs,kparities);

    float leastweight = weight;


    //unsigned long leastCodeword;
    //unsigned char* leastCodeword = malloc(24*sizeof(unsigned char));
    //unsigned char leastCodeword[24];
    unsigned long long retOpt = 0UL;

    //A is default the least weight decoding
    for(i=0;i<24;i++)
            retOpt = (retOpt)+(codeword[i]<<i);
#ifndef golay
    retOpt = retOpt<<12;
    for(i=0;i<12;i++)retOpt+=(codeParity[i]<<i);
#endif



    //----------------A Odd Quarter Lattice Decoder----------------

    constructHexWord(muOs,y,charwts);;
    weight = minH6(y,charwts,muOs);

    weight = hparity(weight,y,prefRepO,dijs,1,codeword);//byref
//    printf("\t");
//    for(d = 0;d<6;d++)printf("%i ", y[d]);
    weight =kparity(weight,codeword,0,codeParity,dijks,dijs,kparities);


    if(weight<leastweight)
    {
        leastweight = weight;
        retOpt = 0UL;
        for(i=0;i<24;i++)
                retOpt = (retOpt)+(codeword[i]<<i);
#ifndef golay
    retOpt = retOpt<<12;
    for(i=0;i<12;i++)retOpt+=(codeParity[i]<<i);
#endif


       winner = 1;
    }

    //----------------H_24 Half Lattice Decoder for B points----------------
    QAM(r,evenBPts,oddBPts,dijs,dijks,kparities);
    blockConf(dijs,muEs,muOs,prefRepE,prefRepO);


    //----------------B Even Quarter Lattice Decoder----------------
    constructHexWord(muEs,y,charwts);
    weight = minH6(y,charwts,muEs);

    weight = hparity(weight,y,prefRepE,dijs,0,codeword);//byref
//    printf("BptEvens\n");
//    printf("\t");
//    for(d = 0;d<6;d++)printf("%i ", y[d]);
    weight =kparity(weight,codeword,1,codeParity,dijks,dijs,kparities);

    if(weight<leastweight){
        retOpt = 0UL;

        leastweight = weight;

        for(i=0;i<24;i++)
                retOpt = (retOpt)+(codeword[i]<<i);
#ifndef golay
    retOpt = retOpt<<12;
    for(i=0;i<12;i++)retOpt+=(codeParity[i]<<i);
#endif


        winner = 2;

    }

    //----------------B Odd Quarter Lattice Decoder----------------
    constructHexWord(muOs,y,charwts);
    weight = minH6(y,charwts,muOs);
    weight = hparity(weight,y,prefRepO,dijs,1,codeword);//byref
//    printf("BptOdds\n");
//    printf("\t");
//    for(d = 0;d<6;d++)printf("%i ", y[d]);
    weight =kparity(weight,codeword,1,codeParity,dijks,dijs,kparities);

    if(weight<leastweight){
        retOpt = 0UL;
        leastweight = weight;
        for(i=0;i<24;i++)
                retOpt = (retOpt)+(codeword[i]<<i);
    #ifndef golay
        retOpt = retOpt<<12;
        for(i=0;i<12;i++)retOpt+=(codeParity[i]<<i);
    #endif


        winner =3;
    }
    //printf("%i\n",winner);
    int c;
    for(c=0;c<4;c++) printf("%i",codeword[c]);printf("\t");
    for(c=0;c<4;c++) printf("%i",codeword[c+4]);printf("\t");
    for(c=0;c<4;c++) printf("%i",codeword[c+8]);printf("\t");
    for(c=0;c<4;c++) printf("%i",codeword[c+12]);printf("\t");
    for(c=0;c<4;c++) printf("%i",codeword[c+16]);printf("\t");
    for(c=0;c<4;c++) printf("%i",codeword[c+20]);printf("\t");
    for(c=0;c<4;c++) printf("%i",codeParity[c]);printf("\t");
    for(c=0;c<4;c++) printf("%i",codeParity[c+4]);printf("\t");
    for(c=0;c<4;c++) printf("%i",codeParity[c+8]);
    printf("\n");

    *distance = winner;
    //*distance += leastweight;
    //free(dijs);
    //free(dijks);
    //free(kparities);
    //free(muEs);
    //free(muOs);
    //free(prefRepO);
    //free(prefRepE);
    //free(y);
    //free(hexword);
    //free(charwts);
    //free(codeword);
    //free(codeParity);
    //free(leastCodeword);





/*
   unsigned long long tem = retOpt;
    while(tem>0){
        printf("%i",tem&1);tem>>=1;
        printf("%i",tem&1);tem>>=1;
        printf( "%i",tem&1);tem>>=1;
        printf( "%i ",tem&1);tem>>=1;
    }printf("\n");*/


    return retOpt;//leastCodeword;
}


int main(int argc, char *argv[]) {
		float dist = 0;
		printf("%u\n",decodeLeech((float[]){-0.98796955, -1.1529434 ,  1.4390883 ,  1.40046597,  0.40183179,
		-0.56885575, -0.81956525,  1.25615557, -1.29526976, -0.62859484,
		-0.7260114 ,  1.19387512,  0.74441283, -0.31003198,  1.16529063,
		0.03210929,  0.88011717,  0.98265615,  1.93322648, -0.05865583,
		-0.56355944, -0.67748379,  0.03904684, -1.0102314},&dist));



		printf("%u\n",decodeLeech((float[]){0.65126908,  0.10690608,  1.16313656,  0.22987196, -1.43084181,
		0.2755519 ,  0.46149737,  1.62229512,  1.84176411,  1.14668634,
		-0.57105783, -0.25542529, -0.30482256,  0.38044721,  1.03321804,
		-0.19284389,  1.07302753, -0.50554365,  1.09262201, -0.17338258,
		-1.12363241, -0.98586661, -0.01722098,  2.4740535},&dist));

		printf("%u\n",decodeLeech((float[]){-1.25952848,  0.90974636,  1.18518797,  0.71649243,  0.00428519,
		0.40136433, -0.44116449,  0.78036808,  1.22932536, -0.4739356 ,
		1.26962219,  0.73379495, -0.37507624,  0.6359808 ,  0.04275665,
		-0.06981256, -2.22652298,  2.10441221,  1.13049073, -0.1140077 ,
		-0.88809368, -1.08038432,  0.73727081,  1.02316672},&dist));



		printf("%u\n",decodeLeech((float[]){-0.06629867,  1.64363637, -0.27086515, -0.37690182, -0.20278382,
		0.84133612, -0.78164611,  0.5310594 ,  0.25187642,  0.56032285,
		-0.43311799,  0.34899539,  1.61118461,  0.82464746, -1.91355652,
		0.48868273, -0.69186852, -0.07240643,  0.16149872,  0.28575778,
		0.13803191, -0.18731954, -0.5343032 ,  0.67212346},&dist));

		printf("%u\n",decodeLeech((float[]){-2.46778603,  1.02194656, -1.21799259,  0.27824392, -0.57911542,
		0.22832422, -1.75776838, -0.09309783,  0.75097076,  0.15962876,
		0.5119343 ,  0.37938917,  0.01796803,  1.03030119, -2.64303921,
		0.32328967,  1.37198716, -0.50753097,  0.47852208,  0.10388366,
		-0.74706363, -0.66855493, -0.35686416,  0.7092663 },&dist));



		printf("%u\n",decodeLeech((float[]){-0.84282073,  0.59923037,  0.73899297,  1.22811334, -0.36589193,
		-0.73147463,  0.31780028,  0.99248704, -0.41232863, -0.34636915,
		0.17348888, -0.93814914, -0.05100204,  1.1133043 , -0.48937103,
		0.28450671,  0.20654879, -2.08840144, -1.72441501, -0.66277794,
		-0.72239422, -0.44093551,  1.02989744,  1.28223695},&dist));



	return 0;
}

