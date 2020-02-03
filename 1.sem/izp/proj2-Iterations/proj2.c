/*
 * Subor:   proj2.c
 * Datum:   November 2016
 * Verzia:  5
 * Autor:   Tomas Nereca, xnerec00@stud.fit.vutbr.cz
 * Projekt: Iteracne vypocty
 * Popis:   Program vykonava vypocet prirodzeneho logaritmu
 						a exponencialnej funkcie s vseobecnym zakladom
						pomocou matematickych operacii +,-,*,/.
 */

// vseobecne funkcie jazyka C
#include <stdlib.h>

// praca so vstupom/vystupom
#include <stdio.h>

// typ bool, konstanty true, false
#include <stdbool.h>

// testovanie znakov - isalpha, isdigit, atd.
#include <ctype.h>

// matematicke funkcie
#include <math.h>

// operacie s retazcami
#include <string.h>

/**
  *Funkcia, ktora je volana v pripade ze nastala chyba argumentov
  *vracia EXIT_SUCCESS
  */

int isError( void )
{
    printf("Nastala chyba pri spracovavani argumentov!\n");
    printf("Program sa spusta v podobe:\n");
    printf("./proj2 --log X N   //(X-logaritmovane cislo, N-pocet iteracii>0)\nalebo\n");
		printf("./proj2 --pow X Y N   //(X-vseobecny zaklad, Y-funkcia z cisla, N-pocet iteracii>0)\n");
    return EXIT_SUCCESS;
}

/**
  *Funkcia testuje, ci je retazec decimalne cislo
  *parameter ch_argument - testovany retazec
  *parameter bl_druh - true ak testujem unsigned int, false ak testujem double
  *vracia 0 v prípade chyby, 1 ak je to cislo v ocakavanom tvare
  */

int isNumber (char* ch_argument, bool bl_druh)
{
  bool bl_digit = false;
  bool bl_bodka = false;

  if (bl_druh)
  {
	  if (isdigit(ch_argument[0]) || ch_argument[0] =='-' || ch_argument[0]=='+')
	  {
		  if (isdigit(ch_argument[0]))
      {
        bl_digit = true;
      }
	  }
    else
    {
      return 0;
    }

	  for (int i = 1; ch_argument[i]!='\0'; i++)
	  {
		  if (!isdigit(ch_argument[i]) && ch_argument[i] != '.')
		  {
			  return 0;
		  }
      else
      {
        if (isdigit(ch_argument[i]))
        {
          bl_digit = true;
        }
        else
        {
          if (bl_bodka == true)
          {
            return 0;
          }
          else
          {
            bl_bodka = true;
          }
        }
      }
	  }
    if (bl_digit == false)
    {
      return 0;
    }
  }
  else
  {
    for (int i = 0; ch_argument[i]!='\0'; i++)
    {
      if (!isdigit(ch_argument[i]))
      {
        return 0;
      }
    }
  }
	return 1;
}

/**
  *Funkcia pocita prirodzeny logaritmus pomocou taylorovho polynomu
  *parameter x - logaritmovane cislo
  *parameter n - pocet iteracii
  *vracia vysledok
  */

double taylor_log(double x, unsigned int n)
{
  if ( x<0 )
  {
    return NAN;
  }
  else
  {
    if( x==0)
    {
      return INFINITY;
    }
    else
    {
	    double cf=0,db_umocnene;
	    if ( x <= 1)
	    {
		    x = 1-x;
		    db_umocnene = x;

        for (unsigned int i = 1;i<=n;i++)
		    {
			    cf = cf -(db_umocnene/i);
			    db_umocnene = db_umocnene*x;
		    }
	    }
	    else
	    {
		    db_umocnene = (x-1)/x;
		    for (unsigned int i = 1;i<=n;i++)
		    {
			    cf = cf +(db_umocnene/i);
			    db_umocnene = db_umocnene*((x-1)/x);
		    }
	    }
      return cf;
    }
  }
}

/**
  *Funkcia pocita prirodzeny logaritmus pomocou zretazeneho zlomku
  *parameter x - logaritmovane cislo
  *parameter n - pocet iteracii
  *vracia vysledok
  */

double cfrac_log(double x, unsigned int n)
{
  n++;
  if ( x<0 )
  {
    return NAN;
  }
  else
  {
    if( x==0)
    {
      return INFINITY;
    }
    else
    {
	    x = (1.0-x)/(-1.0-x);
	    double cf = 1.0;

      for (; n>=1; n--)
	    {
	      cf = (2*n-1) - n*n*x*x/cf;
	    }
	    cf = (2.0*x)/cf;
	    return cf;
    }
  }
}

/**
  *Funkcia pocita exponencialnu funkciu,ln ziskany pomocou taylorovho polynomu
  *parametre x,y - odpovedaju parametrom funkcie pow z matematickej kniznice
  *parameter n - pocet iteracii
  *vracia vysledok
  */

double taylor_pow(double x, double y, unsigned int n)
{
  if ( x<=0 )
  {
    return NAN;
  }
  else
  {
	  double ln = taylor_log(x,n), cf = 1;
    double db_pom = 1;

	  for(unsigned int i = 1; i <= n; i++)
	  {
      db_pom = db_pom * (y*ln/i);
      cf+=db_pom;
	  }
	  return cf;
  }
}

/**
  *Funkcia pocita exponencialnu funkciu,ln ziskany pomocou zretazeneho zlomku
  *parametre x,y - odpovedaju parametrom funkcie pow z matematickej kniznice
  *parameter n - pocet iteracii
  *vracia vysledok
  */

double taylorcf_pow(double x, double y, unsigned int n)
{
  if ( x<=0 )
  {
    return NAN;
  }
  else
  {
	  double ln = cfrac_log(x,n), cf = 1;
    double db_pom = 1;

	  for(unsigned int i = 1; i <= n; i++)
	  {
      db_pom = db_pom * (y*ln/i);
      cf+=db_pom;
	  }
	  return cf;
  }
}

/**
  *Funkcia main sluzi na analyzu argumentov
  *na zaklade argumentov vola funkcie na vypocet a nasledne vypisuje vypocitane hodnoty
  *a hodnoty ziskane z matematickej funkcie
  *v pripade ze boli zadane nespravne argumenty, zavola sa
  *funkcia pre chybove hlasenia a program uspesne skonci
  */

int main( int argc, char* argv[] )
{
  switch(argc)
	{
		case 4:
		{
			if (!strcmp(argv[1],"--log") && isNumber(argv[2], true) && isNumber(argv[3], false))
			{
				printf("       log(%g) = %.12g\n",strtod(argv[2],NULL), log(strtod(argv[2],NULL)));

        printf(" cfrac_log(%g) = %.12g\n",strtod(argv[2],NULL), cfrac_log(strtod(argv[2],NULL),(unsigned int)strtoul(argv[3],NULL,10)));

				printf("taylor_log(%g) = %.12g\n",strtod(argv[2],NULL), taylor_log(strtod(argv[2],NULL),(unsigned int)strtoul(argv[3],NULL,10)));
			}
      else
      {
        return isError();
      }
		}
		break;
		case 5:
		{
			if (!strcmp(argv[1],"--pow") && isNumber(argv[2], true) && isNumber(argv[3], true) && isNumber(argv[4], false) )
			{
        printf("         pow(%g,%g) = %.12g\n",strtod(argv[2],NULL),strtod(argv[3],NULL), pow(strtod(argv[2],NULL),strtod(argv[3],NULL)));

				printf("  taylor_pow(%g,%g) = %.12g\n",strtod(argv[2],NULL),strtod(argv[3],NULL), taylor_pow(strtod(argv[2],NULL),strtod(argv[3],NULL),(unsigned int)strtoul(argv[4],NULL,10)));

				printf("taylorcf_pow(%g,%g) = %.12g\n",strtod(argv[2],NULL),strtod(argv[3],NULL), taylorcf_pow(strtod(argv[2],NULL),strtod(argv[3],NULL),(unsigned int)strtoul(argv[4],NULL,10)));
			}
      else
      {
        return isError();
      }
		}
		break;
		default: return isError();
	}
	return 0;
}
