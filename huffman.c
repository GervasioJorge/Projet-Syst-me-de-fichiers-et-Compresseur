#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TREE_HT 256

typedef struct Node {
    unsigned char data;
    unsigned freq;
    struct Node *left, *right;
} Node;

typedef struct {
    unsigned size;
    Node** array;
} MinHeap;

Node* newNode(unsigned char data, unsigned freq) {
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->left = temp->right = NULL;
    temp->data = data;
    temp->freq = freq;
    return temp;
}

void minHeapify(MinHeap* heap, int idx) {
    int smallest = idx, left = 2 * idx + 1, right = 2 * idx + 2;
    if (left < (int)heap->size && heap->array[left]->freq < heap->array[smallest]->freq) smallest = left;
    if (right < (int)heap->size && heap->array[right]->freq < heap->array[smallest]->freq) smallest = right;
    if (smallest != idx) {
        Node* t = heap->array[smallest];
        heap->array[smallest] = heap->array[idx];
        heap->array[idx] = t;
        minHeapify(heap, smallest);
    }
}

Node* extractMin(MinHeap* heap) {
    if (heap->size == 0) return NULL;
    Node* temp = heap->array[0];
    heap->array[0] = heap->array[heap->size - 1];
    --heap->size;
    if (heap->size > 0) minHeapify(heap, 0);
    return temp;
}

void insertMinHeap(MinHeap* heap, Node* node) {
    unsigned i = heap->size++;
    while (i && node->freq < heap->array[(i - 1) / 2]->freq) {
        heap->array[i] = heap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    heap->array[i] = node;
}

void freeTree(Node* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

void storeCodes(Node* root, int arr[], int top, char* codes[]) {
    if (!root) return;
    if (root->left) { arr[top] = 0; storeCodes(root->left, arr, top + 1, codes); }
    if (root->right) { arr[top] = 1; storeCodes(root->right, arr, top + 1, codes); }
    if (!root->left && !root->right) {
        if (top == 0) {
            // cas spécial : un seul caractère dans le fichier, on lui attribue le code "0"
            codes[(unsigned char)root->data] = (char*)malloc(2);
            codes[(unsigned char)root->data][0] = '0';
            codes[(unsigned char)root->data][1] = '\0';
        } else {
            codes[(unsigned char)root->data] = (char*)malloc(top + 1);
            for(int i=0; i<top; i++) codes[(unsigned char)root->data][i] = arr[i] + '0';
            codes[(unsigned char)root->data][top] = '\0';
        }
    }
}

void compress(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("fopen input"); return; }

    unsigned freq[256] = {0};
    int c;
    while ((c = fgetc(f)) != EOF) freq[(unsigned char)c]++;
    // si fichier vide, on peut pas compresser
    int nonzero = 0;
    for (int i = 0; i < 256; i++) if (freq[i]) nonzero++;
    if (nonzero == 0) { fclose(f); fprintf(stderr, "Fichier vide. Rien à compresser.\n"); return; }

    rewind(f);

    MinHeap heap = {0, (Node**)malloc(256 * sizeof(Node*))};
    for (int i = 0; i < 256; i++) if (freq[i]) insertMinHeap(&heap, newNode((unsigned char)i, freq[i]));

    while (heap.size > 1) {
        Node *l = extractMin(&heap), *r = extractMin(&heap);
        Node *top = newNode('$', l->freq + r->freq);
        top->left = l; top->right = r;
        insertMinHeap(&heap, top);
    }
    Node* root = extractMin(&heap);

    char* codes[256] = {0};
    int arr[MAX_TREE_HT];
    storeCodes(root, arr, 0, codes);

    FILE *out = fopen("compressed.huff", "wb");
    if (!out) { perror("fopen output"); fclose(f); freeTree(root); return; }

    // ecrit les fréquences dans le fichier de sortie (header)
    if (fwrite(freq, sizeof(unsigned), 256, out) != 256) {
        perror("fwrite header"); fclose(f); fclose(out); freeTree(root); return;
    }

    // ecrit les bits de codes dans le fichier de sortie
    unsigned char buffer = 0;
    int bit_count = 0;
    // assume que le fichier d'entrée n'est pas modifié entre les deux passages (ce qui est raisonnable)
    rewind(f);
    while ((c = fgetc(f)) != EOF) {
        char* s = codes[(unsigned char)c];
        if (!s) continue; // doit pas arriver 
        for (int i = 0; s[i]; i++) {
            buffer = (buffer << 1) | (s[i] - '0');
            if (++bit_count == 8) {
                fwrite(&buffer, 1, 1, out);
                buffer = 0; bit_count = 0;
            }
        }
    }
    if (bit_count > 0) { // dernier octet partiellement rempli
        buffer <<= (8 - bit_count);
        fwrite(&buffer, 1, 1, out);
    }

    // nettoyage
    for (int i=0;i<256;i++) if (codes[i]) free(codes[i]);
    free(heap.array);
    freeTree(root);
    fclose(f); fclose(out);
    printf("Compression terminée : compressed.huff créé.\n");
}

void decompress(const char* filename) {
    FILE *in = fopen(filename, "rb");
    if (!in) { perror("fopen input"); return; }
    unsigned freq[256] = {0};
    if (fread(freq, sizeof(unsigned), 256, in) != 256) { perror("fread header"); fclose(in); return; }

    MinHeap heap = {0, (Node**)malloc(256 * sizeof(Node*))};
    unsigned long long total_chars = 0;
    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            insertMinHeap(&heap, newNode((unsigned char)i, freq[i]));
            total_chars += freq[i];
        }
    }
    if (heap.size == 0) { fclose(in); free(heap.array); fprintf(stderr, "Fichier compressé vide ou invalide.\n"); return; }

    while (heap.size > 1) {
        Node *l = extractMin(&heap), *r = extractMin(&heap);
        Node *top = newNode('$', l->freq + r->freq);
        top->left = l; top->right = r;
        insertMinHeap(&heap, top);
    }
    Node *root = extractMin(&heap), *curr = root;

    FILE *out = fopen("decompressed.txt", "wb");
    if (!out) { perror("fopen output"); fclose(in); freeTree(root); free(heap.array); return; }

    // cas spécial : un seul caractère dans le fichier compressé, on écrit ce caractère autant de fois que sa fréquence
    if (!root->left && !root->right) {
        for (unsigned long long i = 0; i < total_chars; i++) fputc(root->data, out);
        fclose(in); fclose(out); freeTree(root); free(heap.array);
        printf("Décompression terminée : decompressed.txt créé.\n");
        return;
    }

    unsigned char buffer;
    while (fread(&buffer, 1, 1, in) == 1 && total_chars > 0) {
        for (int i = 7; i >= 0 && total_chars > 0; i--) {
            int bit = (buffer >> i) & 1;
            curr = bit ? curr->right : curr->left;
            if (!curr) {
                // flux corrompu: on a suivi un chemin qui n'existe pas dans l'arbre
                fclose(in); fclose(out); freeTree(root); free(heap.array);
                fprintf(stderr, "Flux corrompu pendant la décompression.\n");
                return;
            }
            if (!curr->left && !curr->right) {
                fputc(curr->data, out);
                total_chars--;
                curr = root;
            }
        }
    }

    fclose(in); fclose(out); freeTree(root); free(heap.array);
    printf("Décompression terminée : decompressed.txt créé.\n");
}

int main() {
    int choix;
    printf("1. Compresser 'input.txt'\n2. Décompresser 'compressed.huff'\nChoix : ");
    if (scanf("%d", &choix) != 1) return 1;

    if (choix == 1) compress("input.txt");
    else if (choix == 2) decompress("compressed.huff");
    else printf("Choix invalide.\n");
    return 0;
}
