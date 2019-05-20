#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct pixel ///folosim structul asta ca sa salvam pixelii imaginii fie in forma liniarizata (pentru criptare) fie in forma matriceala(pentru PM)
{
    unsigned char B,G,R;
};

struct corelatie///asa pastram ferestrele care au corelatia>0.5
{
    int lin,col;///linia si coloana coltului stanga sus al ferestrei
    double val;///valoarea corelatiei
    char sters;///pentru functia de eliminare a non-maximelor, 0 daca nu a fost sters si 1 daca a fost sters
    struct pixel culo;///culoarea ramei ferestrei

};

//PUNCTUL 1
unsigned int XORSHIFT32(unsigned  int seed)///am preferat sa nu salvez inutil in alt vector valorile xorshift si sa fac functie care returneaza urmatoarea valoare dupa
///"seed"-ul nostru
{
    seed=seed^seed<<13;
    seed=seed^seed>>17;
    seed=seed^seed<<5;
    return seed;
}

//PUNCTUL 2
void incarcareImagine( char *cale,   struct pixel **img,  unsigned char **header,  unsigned int *H,  unsigned int *W )
{
    unsigned int i,j,N;
    unsigned char a;
    FILE *fin=fopen(cale,"rb");

    if(fin==NULL)
    {
        printf("Imaginea nu a putut fi deschisa");
        exit(0);
    }

    *header=(unsigned char *) malloc (54*sizeof(char));///declaram si citim headerul
    fread(*header,sizeof(unsigned char),54,fin);

    rewind(fin);///ne intoarcem la inceputul fisierului

    fseek(fin,18*sizeof(char),SEEK_SET);
    fread(W,sizeof(int),1,fin);///citim Width si Height
    fread(H,sizeof(int),1,fin);
    fseek(fin,54*sizeof(char),SEEK_SET);

    *img=(struct pixel *)malloc(*W * (*H) * 3* sizeof(char)); ///dam W*H*3octeti vectorului liniarizat img

    int octNecesari= (4- (*W*3)%4)%4;///cati octeti sunt necesari pentru a completa o linie sa fie multiplu de 4  (cat padding avem)

    for(i=0; i<*H; ++i)
    {
        N=*W* (*H-i-1);///punem pe "ultima linie" libera pixelii
        for(j=0; j<*W; ++j)///CITIM PIXELII DE PE LINIE
        {
            fread( (*img+N), sizeof(char), 3, fin );///CITIM BGR
            ++N;
        }

        fseek(fin,octNecesari *sizeof(char), SEEK_CUR );///trecem de padding
    }

    fclose(fin);///inchidem fisierul
}


///R0 va fi modificabil deoarece noi vrem sa continuam dupa W*H elemente dupa permutarile noastre de acum (la criptare vom avea nevoie de W*H+1 ... 2*W*H-1)
void permutare(struct pixel **img,unsigned int *R0, unsigned int H, unsigned int W)
{

    unsigned int r,rr,i,aux;

    ///initlializam permutarea "gama"
    unsigned int *permutare;
    permutare=(unsigned int*)malloc(W*H*sizeof(unsigned int));
    for(i=0; i<W*H; ++i)
        permutare[i]=i;

    ///generam permutarea "gama"
    r=XORSHIFT32(*R0);
    for(i=W*H-1; i>=1; --i)///pentru fiecare pixel
    {
        rr=r%(i+1);
        aux=permutare[i];
        permutare[i]=permutare[rr];
        permutare[rr]=aux;

        r=XORSHIFT32(r);
    }

    ///mutam pixelii in functie de permutarea "gama" dar intr-un alt vector "temp"

    struct pixel *temp;

    temp=(struct pixel *)malloc(W*H*sizeof(struct pixel));
    for(i=0; i<W*H; ++i)///pentru fiecare pixel
        temp[permutare[i]]=*(*img+i);

    ///TRANSFERAM PIXELII din temp in img
    for(i=0; i<W*H; ++i)///pentru fiecare pixel
        *(*img+i)=temp[i];


    free(permutare);
    free(temp);

    *R0=r;///actualizam *R0 ca sa continuam de unde am ramas, adica de laa W*H incolo

}


struct pixel xorarePixInt(struct pixel x, unsigned int y)///xorare dintre pixel si integer
{
    unsigned int z=0;
    ///luam octetul 4
    z=y& ((1<<8)-1);
    y=y>>8;
    x.B=(x.B)^z;

    ///luam octetul 3
    z=y& ((1<<8)-1);
    y=y>>8;
    x.G=(x.G)^z;

    ///luam octetul 2
    z=y& ((1<<8)-1);
    y=y>>8;
    x.R=(x.R)^z;
    ///octetul 1 nu va fi luat in considerare
    return x;
}


struct pixel xorarePixPix(struct pixel x, struct pixel y)///xorare dintre doi pixeli
{
    x.B=(x.B)^(y.B);
    x.G=(x.G)^(y.G);
    x.R=(x.R)^(y.R);
    return x;
}

//PUNCTUL 3
void stocare(char *cale, unsigned char *header, struct pixel *img, int H, int W) /// salvam imaginea liniarizata intr-o alta imagine externa
{

    FILE *fout=fopen(cale,"wb");
    int i,j,octNecesari= (4-(W*3)%4) %4 ,N,x=0;


    fwrite(header,sizeof(char),54,fout);///scriem headerul salvat la citirea imaginii liniarizate

    for(i=0; i<H; ++i)
    {
        N=W*(H-i-1);///ne folosim de N pentru a pune pixelii buni la locul lor de veci
        for(j=0; j<W; ++j)
        {
            fwrite(  (img+N) ,sizeof(char),3,fout);
            ++N;
        }

        fwrite(&x,sizeof(char),octNecesari,fout);///punem paddingul

    }

    fclose(fout);
}

//PUNCTUL 4
void criptare(  char *caleImagine, char *caleCriptat, char *caleCheie)
{

    struct pixel *img;
    unsigned int W,H,i;
    unsigned int R0,SV;
    unsigned char *header;

    FILE *f=fopen(caleCheie,"r");
    if(f==NULL)
    {
        printf("Imaginea %s nu a putut fi deschisa", img);
        exit(0);
    }
    fscanf(f,"%d %d",&R0,&SV);

    incarcareImagine(caleImagine,&img,&header,&H,&W);
    permutare(&img,&R0,H,W);


    ///criptarea propriu-zisa

    ///facem primul element
    *img=xorarePixInt(*img,SV);
    *img=xorarePixInt(*img,R0);
    R0=XORSHIFT32(R0);

    ///facem restul elementelor
    for(i=1; i<W*H; ++i)
    {
        *(img+i)=xorarePixPix( *(img+i-1),*(img+i));
        *(img+i)=xorarePixInt( *(img+i),R0);
        R0=XORSHIFT32(R0);
    }

    stocare(caleCriptat,header,img,H,W);

    free(img);
    free(header);
    fclose(f);
}

//PUNCTUL 5
void decriptare(char *caleImagine,char *caleDecriptat, char *caleCriptat, char *caleCheie)
{
    struct pixel *img,*temp;
    unsigned int W, H, i, R0, SV, *header, r, rr, aux, *permutare,*inversa,*R,CR0;
    FILE *f=fopen(caleCheie,"r");
    if(f==NULL)
    {
        printf("Imaginea nu a putut fi deschisa");
        exit(0);
    }

    fscanf(f,"%d %d",&R0,&SV);
    incarcareImagine(caleCriptat,&img,&header,&H,&W);

    ///cream permutarea "gama"
    ///initializam permutarea "gama"
    permutare=(unsigned int*) malloc (W*H*sizeof(unsigned int));

    for(i=0; i<W*H; ++i)
        permutare[i]=i;
    ///generam permutarea "gama"
    R0=XORSHIFT32(R0);
    for(i=W*H-1; i>=1; --i)///pentru fiecare pixel
    {
        rr=R0%(i+1);///luam restul

        aux=permutare[i];
        permutare[i]=permutare[rr];
        permutare[rr]=aux;

        R0=XORSHIFT32(R0);///trecem la urmatorul numar din sirul nostru xorshift

    }
    ///aplicam inversa relatiei

    ///luam R-urile de la W*H pana la 2*W*H-1 intr-un vector separat deoarece noi vrem R-urile sa le luam de la 2*W*H-1 la W*H (DESCRESCATOR)
    R=(unsigned int*)malloc(W*H*sizeof(unsigned int));
    CR0=R0;///pastram R(W*H) ca sa refacem si primul pixel la final
    R0=XORSHIFT32(R0);
    for(i=1; i<W*H; ++i)
    {
        *(R+i)=R0;
        R0=XORSHIFT32(R0);
    }

    ///FACEM ELEMENTELE DE LA FINAL LA INCEPUT deoarece avem nevoie de elementul anterior celui curent la XORARE. Daca luam crecator cel anterior se schimba
    for(i=W*H-1; i>=1; --i)
    {
        *(img+i)=xorarePixPix( *(img+i-1),*(img+i));
        *(img+i)=xorarePixInt( *(img+i),*(R+i));
    }

    *img=xorarePixInt(*img,SV);///refacem culoarea primului pixel
    *img=xorarePixInt(*img,CR0);

    ///aplicam inversa permutarii
    ///mutam pixelii in functie de permutarea "gama" dar intr-un alt vector "img"

    temp=(struct pixel *) malloc (W*H*sizeof(struct pixel));

    for(i=0; i<W*H; ++i)///pentru fiecare pixel
        temp[i]=*(img+permutare[i]);

    ///TRANSPUNEM PIXELII din temp in img
    for(i=0; i<W*H; ++i)///pentru fiecare pixel
        *(img+i)=temp[i];
    stocare(caleDecriptat,header,img,H,W);

    free(temp);
    free(img);
    free(permutare);
    free(header);
    free(R);
    fclose(f);
}

//PUNCTUL 6
void chiPatrat(char *caleImagine)
{
    struct pixel *img,*header;
    double chi,fBarat,*f,numarator;
    unsigned int i,H,W;
    incarcareImagine(caleImagine,&img,&header,&H,&W);

    printf("Imaginea (%s) are urmatoarele valori ale testului chiPatrat:\n",caleImagine);
    f=(double *)malloc(256*sizeof(double));


    //In f vom pastra frecventa unei culori pentru fiecare canal de culoare

    ///frecventa pe Rosu
    for(i=0; i<256; ++i)
        f[i]=0;

    for(i=0; i<W*H; ++i)
        ++f[ (img[i]).R];

    //fbarat este valoarea estimata teoretic pentru fiecare ton (0...255) al culorii respective
    fBarat=W*H/256;
    chi=0;///aici e valoarea lui Chi

    for(i=0; i<256; ++i)
    {
        numarator=f[i]-fBarat;
        chi+=  numarator*numarator/fBarat;

    }

    printf("Rosu: %.2f  \n",chi);


    ///frecventa pe Galben
    for(i=0; i<256; ++i)
        f[i]=0;

    for(i=0; i<W*H; ++i)
        ++f[ (img[i]).G];

    fBarat=W*H/256;
    chi=0;

    for(i=0; i<256; ++i)
    {
        numarator=f[i]-fBarat;
        chi+=  numarator*numarator/fBarat;
    }
    printf("Galben: %.2f  \n",chi);

    ///frecventa pe Albastru
    for(i=0; i<256; ++i)
        f[i]=0;

    for(i=0; i<W*H; ++i)
        ++f[ (img[i]).B];

    fBarat=W*H/256;
    chi=0;

    for(i=0; i<256; ++i)
    {
        numarator=f[i]-fBarat;
        chi+=  numarator*numarator/fBarat;
    }
    printf("Albastru: %.2f  \n\n",chi);
    free(f);
}

void grayscale(struct pixel ***img, unsigned int H, unsigned int W)
{
    int i,j;
    unsigned char aux;
    for(i=0; i<H; ++i)///luam fiecare pixel si ii schimbam culoarea folosind formula
        for(j=0; j<W; ++j)
        {
            aux=0.299* ((*img)[i][j].R) +  0.587* ((*img)[i][j].G) + 0.114 * ((*img)[i][j].B);
            (*img)[i][j].B = (*img)[i][j].R = (*img)[i][j].G=aux;

        }
}

//PUNCTUL 8
void colorare(struct pixel ***img,int lin, int col, unsigned int HS,unsigned int WS,struct pixel culoare)
{
    int i,j;

    for(i=lin; i<lin+HS; ++i)///facem inaaltimea din stanga si cea din dreapta culoarea respectiva
    {
        (*img)[i][col].B=(*img)[i][col+WS-1].B =culoare.B;
        (*img)[i][col].R=(*img)[i][col+WS-1].R=culoare.R;
        (*img)[i][col].G=(*img)[i][col+WS-1].G=culoare.G;
    }


    for(j=col; j<col+WS; ++j)///facem marginile de sus si jos culoarea respectiva
    {
        (*img)[lin][j].B=(*img)[lin+HS-1][j].B =culoare.B;
        (*img)[lin][j].R=(*img)[lin+HS-1][j].R=culoare.R;
        (*img)[lin][j].G=(*img)[lin+HS-1][j].G=culoare.G;
    }

}

void incarcareImagineMatrice( char *cale,   struct pixel ***img,  unsigned char **header,  unsigned int *H,  unsigned int *W )
{

    int i,j,x=0;
    FILE *fin=fopen(cale,"rb");

    if(fin==NULL)
    {
        printf("Imaginea %s nu a putut fi deschisa",cale);
        exit(0);
    }

    *header=(unsigned char *) malloc (54*sizeof(char));
    fread(*header,sizeof(unsigned char), 54, fin);

    rewind(fin);
    ///citim latimea si inaltimea
    fseek(fin,18*sizeof(char),SEEK_SET);
    fread(W,sizeof(int),1,fin);
    fread(H,sizeof(int),1,fin);
    fseek(fin,54*sizeof(char),SEEK_SET);


    ///alocam dinamic matricea
    *img=(struct pixel **) malloc(*H * sizeof(struct pixel *) );

    for(i=0; i<*H; ++i)
        *(*img+i)=(struct pixel *) malloc(*W *sizeof(struct pixel));

    int octNecesari= (4- (*W*3)%4)%4;

    for(i=*H-1; i>=0; --i)///citim liniile in ordine inversa datorita proprietatii fisierelor binare ca pixelii au alta ordine etc
    {
        for(j=0; j<*W; ++j)///CITIM PIXELII DE PE LINIE
            fread(  *(*img+i)+j, sizeof(struct pixel), 1, fin);

        fseek(fin,octNecesari *sizeof(char), SEEK_CUR );///trecem de padding

    }

    fclose(fin);
}

//FUNCTIA DE COMPARARE DE LA PUNCTUL 9
int cmp(struct corelatie *a, struct corelatie *b)
{
    if( (*a).val> (*b).val)return -1;
    return 1;
}

//FUNCTIE CARE VERIFICA SUPRAPUNEREA A DOUA CHENARE LA PUNCTUL 10
char suprapunere(struct corelatie a, struct corelatie b) ///1 daca se suprapun, 0 daca nu se suprapun
{
    unsigned int HS=15,WS=11;///marimile sabloanelor
    struct corelatie aux;
    double arieSuprapusa,arieTotala;

    /*reducem cazurile la 2 situatii
     si anume     a x x x     si   x x x a
                  x x x x          x x x x
                  x x b x          x b x x
    */
    if( a.lin>b.lin )///daca a e "sub" b atunci facem swap sa ajungem la cele 2 cazuri de mai sus
    {
        aux=a;
        a=b;
        b=aux;
    }

    if(a.col<b.col)
    {
        int lungime=a.col+WS-b.col;
        int latime=a.lin+HS-b.lin;
        if(lungime>0 &&latime>0)///daca vreuna este <0 atunci cele doua dreptunghiuri nu se intersecteaza
            arieSuprapusa=lungime*latime;
        else return 0;
    }
    else
    {
        int lungime=b.col+WS-a.col;
        int latime=a.lin+HS-b.lin;
        if(lungime>0 &&latime>0)///daca vreuna este <0 atunci cele doua dreptunghiuri nu se intersecteaza
            arieSuprapusa=lungime*latime;
        else return 0;
    }

    arieTotala=2*WS*HS-arieSuprapusa;

    double raport;
    raport=arieSuprapusa/arieTotala;

    if(raport>0.2)return 1;
    return 0;

}

//PUNCTUL 7
void addCorelatii(struct pixel **img, unsigned char *header, char *sablon, unsigned int H,unsigned int W, struct corelatie  **D, double ps, struct pixel culoare,unsigned int *nrCor )
{

    ///LUAM in matrice
    struct pixel **sab;
    unsigned int HS,WS;
    unsigned char *headerS;
    int i,j,k,l;

    incarcareImagineMatrice(sablon,&sab,&headerS,&HS,&WS);
    grayscale(&sab,HS,WS);


    ///facem gama sablon mai intai deoarece asta e mereu acelasi sablon nu are rost sa il facem de fiecare data
    double Sbarat=0,gamaS=0,n=HS*WS;

    for(i=0; i<HS; ++i)
        for(j=0; j<WS; ++j)
            Sbarat=Sbarat+sab[i][j].B ;
    Sbarat=Sbarat/n;///media pixelilor din sablon

    for(i=0; i<HS; ++i)
        for(j=0; j<WS; ++j)
            gamaS=gamaS+ (sab[i][j].B-Sbarat)*(sab[i][j].B-Sbarat);///facem suma patratelor de diferente
    gamaS=gamaS/(n-1);
    gamaS=sqrt(gamaS);///asta e gamaS


    unsigned int HOK=H-HS,WOK=W-WS;
    for(i=0; i<HOK; ++i)
        for(j=0; j<WOK; ++j)
        {

            unsigned int limitaLinie=HS+i,limitaColoana=WS+j;///limitele spatiului in care vom alege coltul stanga sus, nu iesim din el

            double Fbarat=0,gamaF=0,cor=0;

            for(k=i; k<limitaLinie; ++k)
                for(l=j; l<limitaColoana; ++l)
                    Fbarat=Fbarat+img[k][l].B;

            Fbarat=Fbarat/n; ///media pixelilor din fereastra

            for(k=i; k<limitaLinie; ++k)
                for(l=j; l<limitaColoana; ++l)
                    gamaF=gamaF+ (img[k][l].B-Fbarat)*(img[k][l].B- Fbarat);///facem suma patratelor de diferente

            gamaF=gamaF/(n-1);
            gamaF=sqrt(gamaF);///asta e gamaF

            for(k=i; k<limitaLinie; ++k)
                for(l=j; l<limitaColoana; ++l)
                    cor=cor+(img[k][l].B-Fbarat)*(sab[k-i][l-j].B-Sbarat);

            cor=cor/gamaF;///gamaF si gamaS pot fi considerate constante care ies in afara formulei pentru eficienta
            cor=cor/gamaS;
            cor=cor/n;

            if(cor>ps)///daca corelatia este >pragul nostru atunci ALL IS GUCCI, bagam la vector
            {
                ++(*nrCor);
                *D= (struct corelatie*)realloc(*D,*nrCor*sizeof(struct corelatie));
                (*D)[*nrCor-1].col=j;
                (*D)[*nrCor-1].lin=i;
                (*D)[*nrCor-1].sters=0;
                (*D)[*nrCor-1].val=cor;
                (*D)[*nrCor-1].culo=culoare;

            }

        }
}

//PUNCTUL 10
void eliminareaNonMaximelor(struct corelatie **D,unsigned int nrCor)
{
    int i,j;
    for(i=0; i<nrCor; ++i)

        if( (*D)[i].sters ==0)

            for(j=i+1; j<nrCor; ++j)
                if( (*D)[j].sters==0 && suprapunere( (*D)[i],(*D)[j]) )
                    (*D)[j].sters=1;
}


int main()
{   //PRIMA PARTE

    char *cheie,*imagine,*destinatie;
    cheie=(char*)malloc(10000*sizeof(char));
    imagine=(char*)malloc(10000*sizeof(char));
    destinatie=(char*)malloc(10000*sizeof(char));

    printf("Introduceti numele imaginii pe care doriti sa o criptati sau decriptati: ");
    scanf("%s",imagine);

    printf("\nIntroduceti cheia imaginii pe care doriti sa o criptati sau decriptati: ");
    scanf("%s",cheie);

    printf("\nIntroduceti numele imaginii destinatie: ");
    scanf("%s",destinatie);
    criptare(imagine,destinatie,cheie);
    decriptare(imagine,"decriptat.bmp",destinatie,cheie);

    chiPatrat(imagine);
    chiPatrat(destinatie);

    free(cheie);
    free(imagine);
    free(destinatie);



    //A DOUA PARTE


    char *imaginea,*sablon;///imaginea si sablon
    struct corelatie *D;///aici pastram vectorul de corelatii >0.5
    struct pixel **img,*header,culoare;///pastram headerul si pixelii imaginii intr-o matrice, CULOARE e valoarea culorii cu care vom face chenarul
    unsigned int H,W,nrCor=0;
    int i,j,x=0,octNecesari;///pentru afisari

    imaginea=(char*)malloc(10000*sizeof(char));///declaram sablon si imagine AICI PASTRAM denumirile fisierelor de unde vom lua imaginea si sabloanele
    sablon=(char*)malloc(10000*sizeof(char));

    //INCARCAM IMAGINEA
    FILE *fin=fopen("numeFisiere.txt","r");
    fscanf(fin,"%s",imaginea);
    incarcareImagineMatrice(imaginea,&img,&header,&H,&W);

    //VECTORUL DE CORELATII
    D=(struct corelatie*)malloc(sizeof(struct corelatie));///PUNEM UN ELEMENT IN VECTORUL DE CORELATII (  sa fie ;)   )


    grayscale(&img,H,W);///FACEM IMAGINEA GRI
    double ps=0.5;

    //CIFRA 0
    fscanf(fin,"%s",sablon);
    culoare.B=0,culoare.G=0,culoare.R=255;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 1
    fscanf(fin,"%s",sablon);
    culoare.B=0,culoare.G=255,culoare.R=255;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 2
    fscanf(fin,"%s",sablon);
    culoare.B=0,culoare.G=255,culoare.R=0;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 3
    fscanf(fin,"%s",sablon);
    culoare.B=255,culoare.G=255,culoare.R=0;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 4
    fscanf(fin,"%s",sablon);
    culoare.B=255,culoare.G=0,culoare.R=255;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 5
    fscanf(fin,"%s",sablon);
    culoare.B=255,culoare.G=0,culoare.R=0;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 6
    fscanf(fin,"%s",sablon);
    culoare.B=192,culoare.G=192,culoare.R=192;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 7
    fscanf(fin,"%s",sablon);
    culoare.B=0,culoare.G=140,culoare.R=255;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 8
    fscanf(fin,"%s",sablon);
    culoare.B=128,culoare.G=0,culoare.R=128;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);
    //CIFRA 9
    fscanf(fin,"%s",sablon);
    culoare.B=128,culoare.G=0,culoare.R=0;
    addCorelatii(img,header,sablon,H,W,&D,ps,culoare,&nrCor);


    //ACUM SORTAM
    qsort(D,nrCor,sizeof(struct corelatie),cmp);


    //ELIMINAREA MAXIMELOR
    eliminareaNonMaximelor(&D,nrCor);


    //MAI CITIM O DATA MATRICEA INITIALA DEOARECE A NOASTRA E GREYSCALE ca am transmis-o  cu *** ea a fost modificata etc
    incarcareImagineMatrice(imaginea,&img,&header,&H,&W);


    //DESENARE chenare
    for(i=0; i<nrCor; ++i)
        if(!D[i].sters)
            colorare(&img,D[i].lin,D[i].col,15,11,D[i].culo);


    //Scriem imaginea in memoria externa
    FILE *fout=fopen("afisare.bmp","wb");

    fwrite(header, sizeof(unsigned char), 54, fout);
    octNecesari=(4- (W*3)%4)%4;///octeti de padding

    for(i=H-1; i>=0; --i)
    {

        for(j=0; j<W; ++j)///CITIM PIXELII DE PE LINIE
            fwrite(  *(img+i)+j, sizeof(struct pixel), 1, fout);

        fwrite(&x,sizeof(char),octNecesari, fout);///trecem de padding
    }


    free(imagine);
    free(img);
    free(header);
    free(D);
    free(sablon);
    fclose(fin);
    fclose(fout);
    return 0;
}
