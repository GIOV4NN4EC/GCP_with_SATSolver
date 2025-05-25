#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_VERTICES 100
#define MAX_ARESTAS 1000
#define MAX_CORES 10

// =========================
// Funções para conversão GCP -> CNF
// =========================

int nova_variavel(int vertice, int cor, int total_cores) {
    return vertice * total_cores + cor + 1;
}

void converter_gcp_para_cnf() {
    FILE *arquivo_entrada, *arquivo_saida;
    int numero_vertices, numero_cores, numero_arestas;
    int lista_arestas[MAX_ARESTAS][2];
    int indice_aresta, cor1, cor2, vertice_u, vertice_v;

    arquivo_entrada = fopen("entrada.txt", "r");
    if (arquivo_entrada == NULL) {
        printf("Erro ao abrir o arquivo de entrada.\n");
        return;
    }

    fscanf(arquivo_entrada, "GCP %d %d %d\n", &numero_vertices, &numero_cores, &numero_arestas);

    for (indice_aresta = 0; indice_aresta < numero_arestas; indice_aresta++) {
        fscanf(arquivo_entrada, "a %d %d\n", &lista_arestas[indice_aresta][0], &lista_arestas[indice_aresta][1]);
    }

    arquivo_saida = fopen("saida.cnf", "w");
    if (arquivo_saida == NULL) {
        printf("Erro ao abrir o arquivo de saída.\n");
        fclose(arquivo_entrada);
        return;
    }

    int total_variaveis = numero_vertices * numero_cores;
    int total_clausulas = 0;

    fprintf(arquivo_saida, "p cnf %d 0\n", total_variaveis);

    // Condição 1: Cada vértice deve ter pelo menos uma cor 
    for (int vertice = 0; vertice < numero_vertices; vertice++) {
        for (int cor = 0; cor < numero_cores; cor++) {
            fprintf(arquivo_saida, "%d ", nova_variavel(vertice, cor, numero_cores));
        }
        fprintf(arquivo_saida, "0\n");
        total_clausulas++;
    }

    // Condição 2: Cada vértice não pode ter duas cores ao mesmo tempo
    for (int vertice = 0; vertice < numero_vertices; vertice++) {
        for (cor1 = 0; cor1 < numero_cores; cor1++) {
            for (cor2 = cor1 + 1; cor2 < numero_cores; cor2++) {
                fprintf(arquivo_saida, "-%d -%d 0\n",
                    nova_variavel(vertice, cor1, numero_cores),
                    nova_variavel(vertice, cor2, numero_cores));
                total_clausulas++;
            }
        }
    }

    // Condição 3: Vértices adjacentes não podem ter a mesma cor
    for (indice_aresta = 0; indice_aresta < numero_arestas; indice_aresta++) {
        vertice_u = lista_arestas[indice_aresta][0];
        vertice_v = lista_arestas[indice_aresta][1];
        for (int cor = 0; cor < numero_cores; cor++) {
            fprintf(arquivo_saida, "-%d -%d 0\n",
                nova_variavel(vertice_u, cor, numero_cores),
                nova_variavel(vertice_v, cor, numero_cores));
            total_clausulas++;
        }
    }

    rewind(arquivo_saida);
    fprintf(arquivo_saida, "p cnf %d %d\n", total_variaveis, total_clausulas);

    fclose(arquivo_entrada);
    fclose(arquivo_saida);

    printf("O arquivo CNF foi gerado com sucesso como 'saida.cnf'.\n");
}

void traduzir_resultado() {
    FILE *arquivo_resultado;
    int numero_vertices, numero_cores, numero_variaveis;
    char nome_arquivo[100];

    printf("Digite o número de vértices do grafo: ");
    scanf("%d", &numero_vertices);
    printf("Informe o número de cores possíveis: ");
    scanf("%d", &numero_cores);
    printf("Informe o nome do arquivo de resultado SAT: ");
    scanf("%s", nome_arquivo);

    arquivo_resultado = fopen(nome_arquivo, "r");
    if (arquivo_resultado == NULL) {
        printf("Erro ao abrir o arquivo '%s'.\n", nome_arquivo);
        return;
    }

    numero_variaveis = numero_vertices * numero_cores;

    int variavel, valor;

    printf("\nAqui estão as variáveis traduzidas:\n\n");

    for (int i = 1; i <= numero_variaveis; i++) {
        if (fscanf(arquivo_resultado, "x%d = %d\n", &variavel, &valor) != 2) {
            printf("Erro na leitura da variável %d\n", i);
            break;
        }
        if (valor == 1) {
            int vertice = (variavel - 1) / numero_cores;
            int cor = (variavel - 1) % numero_cores;
            printf("Vértice %d -> cor %d\n", vertice, cor);
        }
    }

    fclose(arquivo_resultado);
}

// =========================
// Estruturas para SAT solver
// =========================

typedef struct NoArvore {
    int indice_variavel;
    struct NoArvore *esquerda;
    struct NoArvore *direita;
} NoArvore;

typedef struct {
    int **clausulas;
    int num_clausulas;
    int num_variaveis;
} FormulaCNF;

NoArvore *criar_arvore(int profundidade, int num_variaveis) {
    if (profundidade >= num_variaveis) return NULL;
    NoArvore *no = malloc(sizeof(*no));
    no->indice_variavel = profundidade;
    no->esquerda = criar_arvore(profundidade + 1, num_variaveis);
    no->direita  = criar_arvore(profundidade + 1, num_variaveis);
    return no;
}

void liberar_arvore(NoArvore *no) {
    if (!no) return;
    liberar_arvore(no->esquerda);
    liberar_arvore(no->direita);
    free(no);
}

bool formula_satisfeita(const FormulaCNF *formula, int atribuicao[]) {
    for (int i = 0; i < formula->num_clausulas; i++) {
        int *clausula = formula->clausulas[i];
        bool satisfeita = false;
        for (int j = 0; clausula[j] != 0; j++) {
            int literal = clausula[j];
            int var = abs(literal) - 1;
            bool verdadeira = atribuicao[var] == 1;
            if ((literal > 0 && verdadeira) || (literal < 0 && !verdadeira)) {
                satisfeita = true;
                break;
            }
        }
        if (!satisfeita) return false;
    }
    return true;
}

bool gerar_com_arvore(NoArvore *no, int atribuicao[], const FormulaCNF *formula, bool *solucao) {
    if (*solucao) return true;
    if (!no) {
        if (formula_satisfeita(formula, atribuicao)) *solucao = true;
        return *solucao;
    }
    int i = no->indice_variavel;
    atribuicao[i] = 1;
    gerar_com_arvore(no->esquerda, atribuicao, formula, solucao);
    if (*solucao) return true;
    atribuicao[i] = 0;
    gerar_com_arvore(no->direita, atribuicao, formula, solucao);
    return *solucao;
}

FormulaCNF *ler_arquivo_cnf(const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) { perror("Erro ao abrir arquivo"); return NULL; }

    FormulaCNF *formula = malloc(sizeof(*formula));
    formula->num_clausulas = 0;
    formula->num_variaveis = 0;
    formula->clausulas = NULL;

    char linha[256];
    int capacidade = 0, contador = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (linha[0] == 'c') continue;
        if (linha[0] == 'p') {
            sscanf(linha, "p cnf %d %d", &formula->num_variaveis, &formula->num_clausulas);
            capacidade = formula->num_clausulas;
            formula->clausulas = malloc(sizeof(int*) * capacidade);
            continue;
        }
        if (contador >= capacidade) {
            capacidade = capacidade == 0 ? 4 : capacidade * 2;
            formula->clausulas = realloc(formula->clausulas, sizeof(int*) * capacidade);
        }
        int *clausula = NULL, tam = 0, cap = 0, literal;
        char *token = strtok(linha, " \n");
        while (token != NULL) {
            literal = atoi(token);
            if (literal == 0) break;
            if (abs(literal) > formula->num_variaveis) {
                fprintf(stderr, "Literal %d fora do intervalo\n", literal);
                fclose(arquivo);
                free(clausula);
                free(formula->clausulas);
                free(formula);
                return NULL;
            }
            if (tam >= cap) {
                cap = cap == 0 ? 4 : cap * 2;
                clausula = realloc(clausula, sizeof(int) * cap);
            }
            clausula[tam++] = literal;
            token = strtok(NULL, " \n");
        }
        clausula = realloc(clausula, sizeof(int) * (tam + 1));
        clausula[tam] = 0;
        formula->clausulas[contador++] = clausula;
    }

    fclose(arquivo);
    formula->num_clausulas = contador;
    return formula;
}

void liberar_formula(FormulaCNF *formula) {
    for (int i = 0; i < formula->num_clausulas; i++) free(formula->clausulas[i]);
    free(formula->clausulas);
    free(formula);
}

void resolver_sat() {
    const char *nome_arquivo = "saida.cnf";
    FormulaCNF *formula = ler_arquivo_cnf(nome_arquivo);
    if (!formula) return;

    if (formula->num_clausulas == 0) {
        printf("SAT\n");
        liberar_formula(formula);
        return;
    }

    int *atribuicoes = malloc(sizeof(int) * formula->num_variaveis);
    for (int i = 0; i < formula->num_variaveis; i++) atribuicoes[i] = -1;

    NoArvore *arvore = criar_arvore(0, formula->num_variaveis);
    bool solucao = false;

    gerar_com_arvore(arvore, atribuicoes, formula, &solucao);
    liberar_arvore(arvore);

    FILE *saida = fopen("resultado_sat.txt", "w");
    if (solucao) {
        for (int i = 0; i < formula->num_variaveis; i++) {
            fprintf(saida, "x%d = %d\n", i + 1, atribuicoes[i] == -1 ? 0 : atribuicoes[i]);
        }
        fprintf(saida, "SAT\n");
        printf("SAT\nResultado salvo em 'resultado_sat.txt'.\n");
    } else {
        fprintf(saida, "UNSAT\n");
        printf("UNSAT\n");
    }
    fclose(saida);
    free(atribuicoes);
    liberar_formula(formula);
}

int main() {
    int opcao;
    do {
        printf("\n===== MENU =====\n");
        printf("1. Converter GCP para CNF\n");
        printf("2. Resolver problema SAT\n");
        printf("3. Traduzir resultado SAT\n");
        printf("0. Sair\n");
        printf("Escolha a opção desejada: ");
        scanf("%d", &opcao);

        switch (opcao) {
            case 1: converter_gcp_para_cnf(); break;
            case 2: resolver_sat(); break;
            case 3: traduzir_resultado(); break;
            case 0: printf("Saindo...\n"); break;
            default: printf("Opção inválida.\n");
        }
    } while (opcao != 0);
    return 0;
}
