
/* ******************************* c204.c *********************************** */
/*  Předmět: Algoritmy (IAL) - FIT VUT v Brně                                 */
/*  Úkol: c204 - Převod infixového výrazu na postfixový (s využitím c202)     */
/*  Referenční implementace: Petr Přikryl, listopad 1994                      */
/*  Přepis do jazyka C: Lukáš Maršík, prosinec 2012                           */
/*  Upravil: Kamil Jeřábek, říjen 2017                                        */
/* ************************************************************************** */
/*
** Implementujte proceduru pro převod infixového zápisu matematického
** výrazu do postfixového tvaru. Pro převod využijte zásobník (tStack),
** který byl implementován v rámci příkladu c202. Bez správného vyřešení
** příkladu c202 se o řešení tohoto příkladu nepokoušejte.
**
** Implementujte následující funkci:
**
**    infix2postfix .... konverzní funkce pro převod infixového výrazu 
**                       na postfixový
**
** Pro lepší přehlednost kódu implementujte následující pomocné funkce:
**    
**    untilLeftPar .... vyprázdnění zásobníku až po levou závorku
**    doOperation .... zpracování operátoru konvertovaného výrazu
**
** Své řešení účelně komentujte.
**
** Terminologická poznámka: Jazyk C nepoužívá pojem procedura.
** Proto zde používáme pojem funkce i pro operace, které by byly
** v algoritmickém jazyce Pascalovského typu implemenovány jako
** procedury (v jazyce C procedurám odpovídají funkce vracející typ void).
**
**/

#include "c204.h"

int solved;


/*
** Pomocná funkce untilLeftPar.
** Slouží k vyprázdnění zásobníku až po levou závorku, přičemž levá závorka
** bude také odstraněna. Pokud je zásobník prázdný, provádění funkce se ukončí.
**
** Operátory odstraňované ze zásobníku postupně vkládejte do výstupního pole
** znaků postExpr. Délka převedeného výrazu a též ukazatel na první volné
** místo, na které se má zapisovat, představuje parametr postLen.
**
** Aby se minimalizoval počet přístupů ke struktuře zásobníku, můžete zde
** nadeklarovat a používat pomocnou proměnnou typu char.
*/
void untilLeftPar ( tStack* s, char* postExpr, unsigned* postLen ) {

    char tmp_top = '0';                         // pomocná premenná do ktorej uložíme vrchol zásobníku aby sme zmenšili počet prístupov do štruktúry

    if(stackEmpty(s) == 0) {
            stackTop(s, &tmp_top);              // ak nie je zásobník prázdny vloží do tmp_top vrchol zásobníku
    }
    

    while(tmp_top != '(') {                     // cyklus dokiaľ nie je na vrchu '(' tak vkladáme operátory do výstupného reťazcu

        if(stackEmpty(s) != 0) {                // ukončenie pokiaľ by bol zásobník prázdny
            return;
        }

        else {
            postExpr[*postLen] = tmp_top;       // vloženie vrcholu zásobníka na výstupný reťazec
            (*postLen)++;
            stackPop(s);                        // uvolnenie vloženého operátora zo zásobníku

            if(stackEmpty(s) == 0) {
                stackTop(s, &tmp_top);          // pokiaľ nie je zásobník prázdny načítame nový vrchol zásobníku
            }
        }
    }
    stackPop(s);                                // vymazanie '(' zo zásobníku
}

/*
** Pomocná funkce doOperation.
** Zpracuje operátor, který je předán parametrem c po načtení znaku ze
** vstupního pole znaků.
**
** Dle priority předaného operátoru a případně priority operátoru na
** vrcholu zásobníku rozhodneme o dalším postupu. Délka převedeného 
** výrazu a taktéž ukazatel na první volné místo, do kterého se má zapisovat, 
** představuje parametr postLen, výstupním polem znaků je opět postExpr.
*/
void doOperation ( tStack* s, char c, char* postExpr, unsigned* postLen ) {

    char tmp_top = '0';                 // pomocná premenná do ktorej uložíme vrchol zásobníku aby sme zmenšili počet prístupov do štruktúry

    if(stackEmpty(s) == 0) {
        stackTop(s, &tmp_top);          // ak nie je zásobník prázdny vloží do tmp_top vrchol zásobníku
    }

    if( (c == '(') || stackEmpty(s) || (tmp_top == '(') ) {         // podmienka pokiaľ je nový znák '(' alebo je zásobník prázdny, alebo je na vrchole '(' vloží nový operátor na zásobník
        stackPush(s, c);                                            // vloženie nového operátora na zásobník
    }

    else if(    ( (c == '+') && (c != tmp_top) && (tmp_top != '-') && (tmp_top != '*') && (tmp_top != '/') ) ||         // podmienky pre operátor '+' aby mohol byť vložený 
                ( (c == '-') && (c != tmp_top) && (tmp_top != '+') && (tmp_top != '*') && (tmp_top != '/') ) ||         // podmienky pre operátor '-' aby mohol byť vložený
                ( (c == '*') && (c != tmp_top) && (tmp_top != '/') ) ||                                                 // podmienky pre operátor '*' aby mohol byť vložený
                ( (c == '/') && (c != tmp_top) && (tmp_top != '*') ) ) {                                                // podmienky pre operátor '/' aby mohol byť vložený 
        stackPush(s, c);
    }

    else if( c == ')') {
        untilLeftPar(s, postExpr, postLen);                                                                             // pokiaľ je operátor ')' zavolá funkciu untilLeftPar
    }

    else {
        while( (stackEmpty(s) == 0) && (tmp_top != '(') ) {                                                             // uvolnenie zvyšku zásobníka alebo po najbližšiu '(' ak príde operátor s nižšou prioritou ako je na vrchu
            postExpr[*postLen] = tmp_top;                                                                               // vloženie vrcholu do výstupného pola                                                              
            (*postLen)++;
            stackPop(s);

            if(stackEmpty(s) == 0) {    
                stackTop(s, &tmp_top);
            }
        }

        if( c == '=') {                             // pokiaľ je operátor '=' tak vloží na koniec reťazca '=' a '\0'
            postExpr[*postLen] = c;
            (*postLen)++;
            postExpr[*postLen] = '\0';
        }

        doOperation(s, c, postExpr, postLen);    //rekurzívne volanie ak nie je koniec reťazca
    }
}

/* 
** Konverzní funkce infix2postfix.
** Čte infixový výraz ze vstupního řetězce infExpr a generuje
** odpovídající postfixový výraz do výstupního řetězce (postup převodu
** hledejte v přednáškách nebo ve studijní opoře). Paměť pro výstupní řetězec
** (o velikosti MAX_LEN) je třeba alokovat. Volající funkce, tedy
** příjemce konvertovaného řetězce, zajistí korektní uvolnění zde alokované
** paměti.
**
** Tvar výrazu:
** 1. Výraz obsahuje operátory + - * / ve významu sčítání, odčítání,
**    násobení a dělení. Sčítání má stejnou prioritu jako odčítání,
**    násobení má stejnou prioritu jako dělení. Priorita násobení je
**    větší než priorita sčítání. Všechny operátory jsou binární
**    (neuvažujte unární mínus).
**
** 2. Hodnoty ve výrazu jsou reprezentovány jednoznakovými identifikátory
**    a číslicemi - 0..9, a..z, A..Z (velikost písmen se rozlišuje).
**
** 3. Ve výrazu může být použit předem neurčený počet dvojic kulatých
**    závorek. Uvažujte, že vstupní výraz je zapsán správně (neošetřujte
**    chybné zadání výrazu).
**
** 4. Každý korektně zapsaný výraz (infixový i postfixový) musí být uzavřen 
**    ukončovacím znakem '='.
**
** 5. Při stejné prioritě operátorů se výraz vyhodnocuje zleva doprava.
**
** Poznámky k implementaci
** -----------------------
** Jako zásobník použijte zásobník znaků tStack implementovaný v příkladu c202. 
** Pro práci se zásobníkem pak používejte výhradně operace z jeho rozhraní.
**
** Při implementaci využijte pomocné funkce untilLeftPar a doOperation.
**
** Řetězcem (infixového a postfixového výrazu) je zde myšleno pole znaků typu
** char, jenž je korektně ukončeno nulovým znakem dle zvyklostí jazyka C.
**
** Na vstupu očekávejte pouze korektně zapsané a ukončené výrazy. Jejich délka
** nepřesáhne MAX_LEN-1 (MAX_LEN i s nulovým znakem) a tedy i výsledný výraz
** by se měl vejít do alokovaného pole. Po alokaci dynamické paměti si vždycky
** ověřte, že se alokace skutečně zdrařila. V případě chyby alokace vraťte namísto
** řetězce konstantu NULL.
*/
char* infix2postfix (const char* infExpr) {

    char* postExpr = (char*) malloc(MAX_LEN);       //alokovanie pamäti pre nové výsupné pole

    if(postExpr == NULL) {          // overenie či sa podarila alokácia
        return NULL;
    }

    unsigned postLen = 0;       // pomocná premenná pre počítanie d´´lžky výstupného reťazca

    tStack s;                   // vytvorenie zásobníka
    stackInit(&s);

    for(int i = 0; infExpr[i] != '\0'; i++) {
        if(
            ( (infExpr[i] >= 'a') && (infExpr[i] <= 'z') ) ||           // načítavanie znakov malej abecedy
            ( (infExpr[i] >= 'A') && (infExpr[i] <= 'Z') ) ||           // načítavanie znakov velkej abecedy
            ( (infExpr[i] >= '0') && (infExpr[i] <= '9') ) ) {          // načítavanie čísiel

            postExpr[postLen] = infExpr[i];
            postLen++; 
        }

        else {
            doOperation(&s, infExpr[i], postExpr, &postLen);            // ak načítame operátor voláme funkciu doOperation
        }
    }

    return postExpr;                                // vrátime výsledný reťazec
}

/* Konec c204.c */
