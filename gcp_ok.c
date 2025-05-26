#include <stdio.h>

#define MAX_VERTICES 100
#define MAX_ARESTAS 1000
#define MAX_CORES 10

//Transforma um par (vértice, cor) em uma variável única de SAT
int nova_variavel(int vertice, int cor, int total_cores) {
    return vertice * total_cores + cor + 1;
}

// Função para converter GCP para CNF
void converter_gcp_para_cnf() {
    FILE *arquivo_entrada, *arquivo_saida;
    int numero_vertices, numero_cores, numero_arestas;
    int lista_arestas[MAX_ARESTAS][2];
    int indice_aresta, cor1, cor2, vertice_u, vertice_v;

    // Abre o arquivo de entrada
    arquivo_entrada = fopen("entrada.txt", "r");
    if (arquivo_entrada == NULL) {
        printf("Erro ao abrir o arquivo de entrada.\n");
        return;
    }

    // Lê o cabeçalho: número de vértices, cores e arestas
    fscanf(arquivo_entrada, "GCP %d %d %d\n", &numero_vertices, &numero_cores, &numero_arestas);

    // Lê as arestas
    for (indice_aresta = 0; indice_aresta < numero_arestas; indice_aresta++) {
        fscanf(arquivo_entrada, "a %d %d\n", &lista_arestas[indice_aresta][0], &lista_arestas[indice_aresta][1]);
    }

    // Abre o arquivo de saída CNF
    arquivo_saida = fopen("saida.cnf", "w");
    if (arquivo_saida == NULL) {
        printf("Erro ao abrir o arquivo de saída.\n");
        fclose(arquivo_entrada);
        return;
    }

    int total_variaveis = numero_vertices * numero_cores;
    int total_clausulas = 0;

    // Cabeçalho provisório
    fprintf(arquivo_saida, "p cnf %d 0\n", total_variaveis);

// Condição 1: Cada vértice deve ter pelo menos uma cor 
    for (int vertice = 0; vertice <= numero_vertices; vertice++) { // Para cada um dos vértices
        for (int cor = 0; cor <= numero_cores; cor++) { // Atribui cada uma das cores
            fprintf(arquivo_saida, "%d ", nova_variavel(vertice, cor, numero_cores)); // E gera uma nova variavel correspondente a esse par
            // Além disso, escreve a nova variável no arquivo de saída
        }
        fprintf(arquivo_saida, "0\n");  // Finaliza a cláusula
        total_clausulas++;
    }

    // Condição 2: Cada vértice não pode ter duas cores ao mesmo tempo 
    for (int vertice = 0; vertice <= numero_vertices; vertice++) {// Para cada um dos vértices
        for (cor1 = 0; cor1 <= numero_cores; cor1++) { //percorre todas as cores possiveis para cor 1
            for (cor2 = cor1 + 1; cor2 <= numero_cores; cor2++) { //percorre todas as cores possíveis para cor 2
                //cor2 = cor1+1 para evitar clausulas repetidas
                fprintf(arquivo_saida, "-%d -%d 0\n", // escreve no arquivo de saída com as variáveis já "convertidas"
                    // As variáveis são escritas negadas, pois o vértice pode não ter nenhuma delas, mas não pode ter as duas ao mesmo tempo
                        nova_variavel(vertice, cor1, numero_cores),
                        nova_variavel(vertice, cor2, numero_cores));
                total_clausulas++;
            }
        }
    }

    // Condição 3: Vértices adjacentes não podem ter a mesma cor 
    for (indice_aresta = 0; indice_aresta < numero_arestas; indice_aresta++) {// Percorre as arestas
        //Guarda os dois vertices da aresta
        vertice_u = lista_arestas[indice_aresta][0];
        vertice_v = lista_arestas[indice_aresta][1];

        for (int cor = 0; cor <= numero_cores; cor++) {// Percorre todas as cores
            fprintf(arquivo_saida, "-%d -%d 0\n",
                    nova_variavel(vertice_u, cor, numero_cores),
                    nova_variavel(vertice_v, cor, numero_cores));
            total_clausulas++;
        }
    }

    // Atualiza o cabeçalho com o número correto de cláusulas
    rewind(arquivo_saida);
    fprintf(arquivo_saida, "p cnf %d %d\n", total_variaveis, total_clausulas);

    fclose(arquivo_entrada);
    fclose(arquivo_saida);

    printf("O arquivo CNF foi gerado com sucesso como 'saida.cnf'.\n");
}

// Função para desmapear variáveis a partir do resultado SAT
void traduzir_resultado() {
    FILE *arquivo_resultado;
    int numero_vertices, numero_cores, numero_variaveis;
    char nome_arquivo[100];

    printf("Digite o número de vértices do grafo: ");
    scanf("%d", &numero_vertices);

    printf("Informe o número de cores possíveis: ");
    scanf("%d", &numero_cores);

    printf("Informe o nome do arquivo de resultado SAT (a ser traduzido): ");
    scanf("%s", nome_arquivo);

    arquivo_resultado = fopen(nome_arquivo, "r");
    if (arquivo_resultado == NULL) {
        printf("Erro ao abrir o arquivo '%s'.\n", nome_arquivo);
        return;
    }

    numero_variaveis = numero_vertices * numero_cores;

    int variavel, valor;

    printf("\nAqui estão as variáveis traduzidas:    \n\n");

    for (int i = 1; i <= numero_variaveis; i++) {
        if (fscanf(arquivo_resultado, "x%d = %d\n", &variavel, &valor) != 2) { // Se a função nao recebe dois valores
            printf("Erro na leitura da variável %d, verifique se está digitada corretamente.\n", i);
            break;
        }

        if (valor == 1) {  // Variável verdadeira
            //Faz o processo contrario ao do mapeamento
            int vertice = (variavel - 1) / numero_cores;
            int cor = ((variavel - 1) % numero_cores);

            printf("Vértice %d -> cor %d\n", vertice, cor);
        }
        // Não é necessário desmapear as variáveis falsas, porque elas só indicam que o vértice não tem determinada cor
    }

    fclose(arquivo_resultado);
}

int main() {
    int opcao;

    do {
        printf("\n===== Selecione a opção desejada: =====\n");
        printf("1. Converter arquivo GCP para CNF\n");
        printf("2. Traduzir resultado SAT\n");
        printf("0. Sair\n");
        scanf("%d", &opcao);

        switch (opcao) {
            case 1:
                converter_gcp_para_cnf();
                break;
            case 2:
                traduzir_resultado();
                break;
            case 0:
                printf("Saindo...\n");
                break;
            default:
                printf("Opção inválida.\n");
        }
    } while (opcao != 0);

    return 0;
}
