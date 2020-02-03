/*
 * Subor:   proj1.c
 * Datum:   November 2016
 * Verzia:  12
 * Autor:   Tomas Nereca, xnerec00@stud.fit.vutbr.cz
 * Projekt: Praca s textom
 * Popis:   Program bud binarne data formatuje do textovej podoby
 *          alebo textovu podobu dat prevadza do binarnej podoby
 */

// praca so vstupom/vystupom
#include <stdio.h>

// typ bool, konstanty true, false
#include <stdbool.h>

// testovanie znakov - isalpha, isdigit, atd.
#include <ctype.h>

/**
  *Funkcia, ktora je volana v pripade ze program nemoze uspesne prebehnut
  *parameter bl_typ udava aky typ chyby nastal
  *v pripade chyby argumentov vracia funkcia FALSE
  *v pripade chybneho vstupu vracia funcia TRUE
  */

bool isError( bool bl_typ )
{
  if (bl_typ == 0)
  {
    fprintf(stderr,"Nastala chyba pri spracovavani argumentov!\n");
    fprintf(stderr,"Program sa spusta bez argumentov, alebo v nasledujucej podobe:\n");
    fprintf(stderr, "./proj1 [-s M] [-n N] (M-nezaporne cislo, N-kladne cislo)\n./proj1 -x\n./proj1 -S N (0<N<200)\n./proj1 -r\n");
    return 0;
  }
  else
  {
    fprintf(stderr, "Nastala neocakavana chyba za behu programu. Nespravny format vstupnych dat!\n");
    return 1;
  }
}

/**
  *Funkcia naplna pole znakov
  *parameter ch - znak ktorym naplnam pole
  *parameter int_velkost - velkost pola
  *parameter p_ukazatel - ukazatel na pole ktore naplnam
  */

void fillArray(char ch, int int_velkost, char* p_ukazatel)
{
  for (int i = 0; i < int_velkost; i++)
  {
    p_ukazatel[i] = ch;
  }
}

/**
  *Funkcia na prevod decimalneho cisla ulozeneho v poli znakov (v argumente) na integer
  *parameter argv je pole znakov v ktorom BY MALO byt ulozene nezaporne cislo
  *funckia vracia -1 ak je v argumente ulozene nieco ine ako nezaporne cislo
  *inak vracia funkcia ziskane cislo uz typu integer
 */

int charsToInt(char* argv)
{
    int i = 0,int_exponent = 0,int_cislo = -1;

    while(argv[i] != '\0')
    {
      if ( argv[i] < '0' || argv[i] > '9' )
      {
        return -1;
      }
      else
      {
        i++;
      }
    }

    int_cislo = 0;

    for(int j = 0; j<i; j++)
    {
      int_exponent = 1;
      for (int k = 0; k<j; k++)
      {
        int_exponent = int_exponent*10;
      }
    int_cislo = int_cislo + ((int)argv[i-1-j]-'0') * int_exponent;
    }
  return int_cislo;
}

/**
  *Funkcia na prevod hexadecimalneho cisla ulozeneho v poli znakov na integer
  *parameter ch_cislo - dvojrozmerne pole znakov (hexadecimalne cislo)
  *funkcia vracia decimalne cislo typu integer
  */

int hexCharsToInt(char* ch_cislo)
{
  int int_vysledok = 0, int_pom = 1;
  if (ch_cislo[1] == 'v')
  {
    ch_cislo[1] = ch_cislo[0];
    ch_cislo[0] = '0';
  }
  for (int i = 1; i > -1; i--)
  {
    if(ch_cislo[i] >= '0' && ch_cislo[i] <= '9')
    {
      int_vysledok += (int_pom*(ch_cislo[i] - '0'));
    }
    else
    {
      if (ch_cislo[i] >= 'a' && ch_cislo[i] <= 'f')
      {
        int_vysledok += (int_pom*(ch_cislo[i] - 'a' + 10));
      }
      else
      {
        int_vysledok += (int_pom*(ch_cislo[i] - 'A' + 10));
      }
    }
    int_pom*=16;
  }
  return int_vysledok;
}

/**
  *Funkcia, ktora je volana pri spusteni programu bez argumentov alebo s arguentmi [-s M] [-n N]
  *vstupne binarne data prevadza do textovej podoby
  *vypise adresu, postupnost 16 bajtov (hexadecimalne cisla) a textovu podobus
  *argument int_preskocit - nezaporne cislo pri argumente -s
  *argument int_maxdlzka - kladne cislo pri argumente -n
  */

void basic(int int_preskocit, int int_maxdlzka)
{
    unsigned int uint_adresa = 0;
    int sh_znak = getchar(), int_pocet = 0;
    char ch_tisk[17];
    ch_tisk[16] = '\0';

    for (int i=0; i<int_preskocit; i++)
    {
      sh_znak = getchar();
      uint_adresa++;
    }

    while (sh_znak != EOF && (int_maxdlzka == 0 || int_pocet < int_maxdlzka))
    {
        printf("%.8x ", uint_adresa);
        fillArray(' ', 16, ch_tisk);
        for(int i = 0; (i<2); i++)
        {
          for (int j = 0; (j<8); j++)
          {
            if (sh_znak != EOF && (int_maxdlzka == 0 || int_pocet < int_maxdlzka))
            {
                printf("%.2x ", sh_znak);
                  uint_adresa++;
                  int_pocet++;
                if (isprint(sh_znak))
                {
                  ch_tisk[(i*8)+j] = sh_znak;
                }
                else
                {
                  ch_tisk[(i*8)+j] = '.';
                }
                sh_znak = getchar();
            }
            else
            {
              printf("   ");
            }
          }
          printf(" ");
        }
        printf("|%s|\n", ch_tisk);
    }
}

/**
  *Funkcia, ktora je volana pri spusteni programu s argumentom -x
  *vsetky vstupne data su prevedene do hexadecimalnej podoby na jeden riadok
  *kazdemu bajtu odpoveda dvojciferne hexadecimalne cislo
  *na konci funkcia odriadkuje
  */

void toHex(void)
{
  int int_znak;
  int_znak = getchar();
  while (int_znak != EOF)
  {
    printf("%.2x", int_znak);
    int_znak = getchar();
  }
  printf("\n");
}

/**
  *Funkcia, ktora je volana pri spusteni programu s argumentom -r
  *na vstupe ocakava sekvenciu hexadecimalnych cislic, ignoruje biele znaky
  *vracia -1 v pripade chybneho vstupu
  */

int reverse(void)
{
  int int_znak = getchar();
  char ch_cislo[2] = {'v','v'};
  while ( int_znak != EOF )
  {
    if (isspace(int_znak))
    {
      int_znak = getchar();
      continue;
    }
    if ((int_znak>='a' && int_znak<='f') || (int_znak>='A' && int_znak<='F') || (int_znak>='0' && int_znak<='9'))
    {
        if (ch_cislo[0] == 'v')
        {
          ch_cislo[0] = int_znak;
        }
        else
        {
          ch_cislo[1] = int_znak;
        }
    }
    else
    {
      return -1;
    }
    if ((ch_cislo[0] != 'v') && (ch_cislo[1] != 'v'))
    {
      printf("%c", hexCharsToInt(ch_cislo));
      ch_cislo[0] = ch_cislo[1] = 'v';
    }
    int_znak = getchar();
  }
  if (ch_cislo[0] != 'v')
  {
    printf("%c", hexCharsToInt(ch_cislo));
  }
  return 0;
}

/**
  *Funkcia, ktora je volana pri spusteni programu s argumentom -S N
  *tlaci len take postupnosti v binarnom subore, ktore vyzeraju ako retazec
  *int_minDlzka - najmensia pripustna dlzka retazca
  */

void strings(int int_minDlzka)
{
  char ch_pole[int_minDlzka+1];
  int int_znak = getchar();
  short sh_pocet = 0;
  fillArray('\0', int_minDlzka+1, ch_pole);
  bool bl_odriadkuj;

  while (int_znak != EOF)
  {
    bl_odriadkuj = 1;
    if (isprint(int_znak) || isblank(int_znak))
    {
      if (sh_pocet < int_minDlzka)
      {
        ch_pole[sh_pocet] = int_znak;
        sh_pocet++;
      }
      else
      {
        if (sh_pocet == int_minDlzka)
        {
          printf("%s%c", ch_pole, int_znak);
          sh_pocet++;
        }
        else
        {
          printf("%c", int_znak);
          sh_pocet++;
        }
      }
    }
    else
    {
      if (sh_pocet == int_minDlzka)
      {
        printf("%s\n", ch_pole);
      }
      else
      {
        if (sh_pocet > int_minDlzka)
        {
          printf("\n");
        }
      }
      fillArray('\0', int_minDlzka+1, ch_pole);
      sh_pocet = 0;
      bl_odriadkuj = 0;
    }
    int_znak = getchar();
  }
  if(bl_odriadkuj == 1)
  {
    printf("\n");
  }
}

/**
  *Funkcia main sluzi na analyzu argumentov
  *na zaklade argumentov vola dalsie funkcie
  *v pripade ze boli zadane nezname argumenty, alebo nastala ina chyba, zavola sa
  *funkcia pre chybove hlasenia a program skonci
  */

int main(int argc, char* argv[])
{
  int int_preskocit = -1,int_dlzka = 0;

    switch (argc)
    {
        case 1:
        {
          basic(int_preskocit, int_dlzka);
        }
        break;
        case 2:
        {
          if ( argv[1][0] == '-' && argv[1][1] == 'x' && argv[1][2] == '\0')
          {
              toHex();
          }
          else
          {
            if ( argv[1][0] == '-' && argv[1][1] == 'r' && argv[1][2] == '\0')
            {
              if (reverse() == -1)
              {
                return isError(1);
              }
            }
            else
            {
              return isError(0);
            }
          }
        }
        break;
        case 3:
        {
          if (argv[1][0] == '-' && argv[1][1] == 'S' && argv[1][2] == '\0')
          {
            int_dlzka = charsToInt (argv[2]);
            if (int_dlzka > 0 && int_dlzka < 200)
            {
              strings(int_dlzka);
            }
            else
            {
              return isError(0);
            }
          }
          else
          {
            if ( argv[1][0] == '-' && argv[1][1] == 'n' && argv[1][2] == '\0')
            {
              int_dlzka = charsToInt(argv[2]);
              if (int_dlzka <= 0)
              {
                return isError(0);
              }
              else
              {
                basic(int_preskocit, int_dlzka);
              }
            }
            else
            {
              if ( argv[1][0] == '-' && argv[1][1] == 's' && argv[1][2] == '\0')
              {
                int_preskocit = charsToInt(argv[2]);
                if (int_preskocit < 0)
                {
                  return isError(0);
                }
                else
                {
                  basic(int_preskocit, int_dlzka);
                }
              }
              else
              {
                return isError(0);
              }
            }
          }
        }
        break;
        case 5:
        {
          if((argv[1][0] == '-' && argv[1][1] == 'n' && argv[1][2] == '\0') && (argv[3][0] == '-' && argv[3][1] == 's' && argv[3][2] == '\0'))
            {
              int_dlzka = charsToInt(argv[2]);
              int_preskocit = charsToInt(argv[4]);
              if (int_preskocit < 0 || int_dlzka <= 0 )
              {
                  return isError(0);
              }
              else
              {
                basic(int_preskocit, int_dlzka);
              }
            }
            else
            {
              if((argv[1][0] == '-' && argv[1][1] == 's' && argv[1][2] == '\0') && (argv[3][0] == '-' && argv[3][1] == 'n' && argv[3][2] == '\0'))
              {
                int_preskocit = charsToInt(argv[2]);
                int_dlzka = charsToInt(argv[4]);
                if (int_preskocit < 0 || int_dlzka <= 0 )
                {
                    return isError(0);
                }
                else
                {
                    basic(int_preskocit, int_dlzka);
                }
              }
              else
              {
                return isError(0);
              }
            }
          }
        break;
        default: return isError(0);
    }

return 0;
}
