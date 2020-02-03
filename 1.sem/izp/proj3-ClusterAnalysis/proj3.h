/**
 * Kostra hlavickoveho souboru 3. projekt IZP 2015/16
 * a pro dokumentaci Javadoc.
 */

 /**
  * Program vykonava jednoduchu zhlukovu analyzu podla metody najvzdialenejsieho suseda
  * @file
  * @author Tomas Nereca <xnerec00>
  */



struct obj_t {
    int id;
    float x;
    float y;
};

struct cluster_t {
    int size;
    int capacity;
    struct obj_t *obj;
};

/**
 * Funkcia inicializuje zadany zhluk - alokuje pamat pre pozadovanu kapacitu objektov
 * @param c ukazatel na zhluk, ktory bude funkcia inicializovat
 * @param cap kapacita - pocet objektov, pre ktore sa alokuje pamat
 * @returns nic
 */

void init_cluster(struct cluster_t *c, int cap);

/**
 * Funkcia odstranuje vsetky objekty v pozadovanom zhluku a inicializuje ho na prazdny zhluk
 * @param c ukazatel na zhluk, s ktorym bude funkcia pracovat
 * @returns nic
 */

void clear_cluster(struct cluster_t *c);

extern const int CLUSTER_CHUNK;

/**
 * Funkcia meni kapacitu zhluku - realokuje pamat pre pozadovany pocet objektov
 * @param c ukazatel na zhluk, s ktorym bude funkcia pracovat
 * @param new_cap pozadovana nova kapacita zhluku - pocet objektov, pre ktore je alokovana pamat
 * @returns ukazatel na zhluk so zmenenou kapacitou
 */

struct cluster_t *resize_cluster(struct cluster_t *c, int new_cap);

/**
 * Funkcia pridava objekt na koniec zhluku, tzn. za posledny objekt
 * Ak sa velkost zhluku rovna kapacite, zhluk bude rozsireny
 * @param c  ukazatel na zhluk, s ktorym bude funkcia pracovat
 * @param new_cap pozadovana nova kapacita zhluku - pocet objektov, pre ktore je alokovana pamat
 * @returns nic
 */

void append_cluster(struct cluster_t *c, struct obj_t obj);

/**
 * Funkcia pridava vsetky objekty jedneho zhluku do druheho
 * V pripade potreby bude zhluk, do ktoreho sa pridavaju objekty, rozsireny
 * Objekty v zhluku sa zoradia vzostupne podla identifikacneho cisla
 * @param c1 ukazatel na zhluk do ktoreho sa pridavaju objekty
 * @param c2 ukazatel na zhluk z ktoreho su pridavane objekty, zostava nezmeneny
 * @returns nic
 */

void merge_clusters(struct cluster_t *c1, struct cluster_t *c2);

/**
 * Funkcia odstranuje konkretny zhluk z pola zhlukov
 * @param carr ukazatel na prvy zhluk v poli zhlukov
 * @param narr pocet vsetkych zhlukov v poli
 * @param idx index to zhluku, ktory ma byt odstraneny z pola
 * @returns novy pocet zhlukov v poli
 */

int remove_cluster(struct cluster_t *carr, int narr, int idx);

/**
 * Funkcia pocita Euklidovsku vzdialenost dvoch zadanych objektov
 * @param o1 ukazatel na prvy objekt
 * @param o2 ukazatel na druhy objekt
 * @returns Euklidovsku vzdialenost dvoch objektov
 */

float obj_distance(struct obj_t *o1, struct obj_t *o2);

/**
 * Funkcia pocita vzdialenost dvoch zhlukov podla metody najvzdialenejsieho suseda
 * @param c1 prvy zhluk
 * @param c2 druhy zhluk
 * @returns vzdialenost dvoch zhlukov
 */

float cluster_distance(struct cluster_t *c1, struct cluster_t *c2);

/**
 * Funkcia hlada dva najblizsie zhluky v zadanom poli zhlukov o zadanej velkosti podla metody najvzdialenejsieho suseda
 * a uklada 2 indexy - 2 najblizsie zhluky
 * @param carr ukazatel na prvu polozku v poli zhlukov
 * @param narr pocet prvkov v poli zhlukov
 * @param c1 ukazatel na cislo - jeden index z pola zhlukov
 * @param c2 ukazatel na cislo - druhy index z pola zhlukov
 * @returns nic
 */

void find_neighbours(struct cluster_t *carr, int narr, int *c1, int *c2);

/**
 * Funkcia zoraduje objekty v zadanom zhluku vzostupne podla ich jedinecneho identifikatoru - cisla
 * @param c ukazatel na zhluk, v ktorom maju byt zoradene zhluky
 * @returns nic
 */

void sort_cluster(struct cluster_t *c);

/**
 * Funkcia tlaci obsah zadaneho zhluku (identifikator, x-ova suradnica, y-ova suradnica) na stdout
 * @param c ukazatel na zhluk, ktory ma byt vytlaceny
 * @returns nic
 */

void print_cluster(struct cluster_t *c);

/**
 * Funkcia tlaci obsah zadaneho zhluku (identifikator, x-ova suradnica, y-ova suradnica) na stdout
 * @param c ukazatel na zhluk, ktory ma byt vytlaceny
 * @returns nic
 */

int load_clusters(char *filename, struct cluster_t **arr);

/**
 * Funkcia tlaci zadane pole zhlukov na stdout
 * @param c ukazatel na prvu polozku pola zhlukov
 * @returns nic
 */

void print_clusters(struct cluster_t *carr, int narr);
