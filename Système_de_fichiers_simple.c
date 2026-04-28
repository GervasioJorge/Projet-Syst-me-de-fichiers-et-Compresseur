#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Fichier { /* représentation fichier*/
    char nom[64];
    char* contenu;
    int taille;
} Fichier;

typedef struct Repertoire { /* représentation répertoire/arbre: tableau de pointeurs */
    char nom[64];
    struct Repertoire* parent;
    struct Repertoire** sous_repertoires;
    int nb_repertoires;
    Fichier** fichiers;
    int nb_fichiers;
} Repertoire;


Fichier* creer_fichier(const char* nom, const char* contenu) {
    Fichier* nouveau_fichier = malloc(sizeof(Fichier)); //on ne connait pas la taille du contenu 
    strncpy(nouveau_fichier->nom, nom, 63);
    nouveau_fichier->taille = strlen(contenu);
    nouveau_fichier->contenu = strdup(contenu); // Allocation dynamique du contenu
    return nouveau_fichier;
}

Repertoire* creer_repertoire(const char* nom, Repertoire* parent) {
    Repertoire* nouveau_rep = malloc(sizeof(Repertoire));
    strncpy(nouveau_rep->nom, nom, 63);
    nouveau_rep->parent = parent;
    nouveau_rep->sous_repertoires = NULL;
    nouveau_rep->nb_repertoires = 0;
    nouveau_rep->fichiers = NULL;
    nouveau_rep->nb_fichiers = 0;
    return nouveau_rep;
}



void ajouter_fichier_a_rep(Repertoire* rep, const char* nom, const char* contenu) {
    rep->fichiers = realloc(rep->fichiers, sizeof(Fichier*) * (rep->nb_fichiers + 1));
    rep->fichiers[rep->nb_fichiers] = creer_fichier(nom, contenu);
    rep->nb_fichiers++;
}

void ajouter_sous_rep_a_rep(Repertoire* rep, const char* nom) {
    rep->sous_repertoires = realloc(rep->sous_repertoires, sizeof(Repertoire*) * (rep->nb_repertoires + 1));
    rep->sous_repertoires[rep->nb_repertoires] = creer_repertoire(nom, rep);
    rep->nb_repertoires++;
}

void renomer_fichier(Fichier* fichier, const char* nouveau_nom) {
    strncpy(fichier->nom, nouveau_nom, 63);
}

void deplacer_fichier(Repertoire* source, Repertoire* destination, const char* nom_fichier) {
    for (int i = 0; i < source->nb_fichiers; i++) {
        if (strcmp(source->fichiers[i]->nom, nom_fichier) == 0) {
            ajouter_fichier_a_rep(destination, source->fichiers[i]->nom, source->fichiers[i]->contenu);
            free(source->fichiers[i]->contenu);
            free(source->fichiers[i]);
            for (int j = i; j < source->nb_fichiers - 1; j++) {
                source->fichiers[j] = source->fichiers[j + 1];
            }
            source->nb_fichiers--;
            return;
        }
    }
    printf("Fichier '%s' non trouvé dans le répertoire '%s'.\n", nom_fichier, source->nom);
}

void liberer_repertoire(Repertoire* rep) {
    for (int i = 0; i < rep->nb_fichiers; i++) {
        free(rep->fichiers[i]->contenu);
        free(rep->fichiers[i]);
    }
    free(rep->fichiers);

    for (int i = 0; i < rep->nb_repertoires; i++) {
        liberer_repertoire(rep->sous_repertoires[i]);
    }
    free(rep->sous_repertoires);
    free(rep);
}



int main() {
    // Initialisation de la racine
    Repertoire* racine = creer_repertoire("racine", NULL);
    Repertoire* courant = racine;

    
    ajouter_sous_rep_a_rep(courant, "maison");
    ajouter_fichier_a_rep(courant, "note.txt", "Bonjour Monde");

    printf("Système de fichiers initialisé.\n");
    printf("Répertoire courant : %s\n", courant->nom);
    printf("Fichier créé : %s (Taille : %d)\n", courant->fichiers[0]->nom, courant->fichiers[0]->taille);

    liberer_repertoire(racine);
    return 0;
}
