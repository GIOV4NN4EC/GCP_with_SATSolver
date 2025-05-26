#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_VERTICES 100
#define MAX_ARESTAS 1000
#define MAX_CORES 10

//Tradutor GCP -> CNF

//gerador de variaveis com base no par (vertice x, cor y)
int nova_variavel(int vertice, int cor, int total_cores) {
    return vertice * total_cores + cor + 1;
    //+1 pois o formato DIMACS só aceita variaveis a partir de 1
}

//Função principal do tradutor
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

    //Lê o cabeçalho do arquivo com o GCP
    fscanf(arquivo_entrada, "GCP %d %d %d\n", &numero_vertices, &numero_cores, &numero_arestas);

    //Armazena as arestas em uma matriz
    for (indice_aresta = 0; indice_aresta < numero_arestas; indice_aresta++) {
        fscanf(arquivo_entrada, "a %d %d\n", &lista_arestas[indice_aresta][0], &lista_arestas[indice_aresta][1]);
    }

    arquivo_saida = fopen("saida.cnf", "w");
    if (arquivo_saida == NULL) {
        printf("Erro ao abrir o arquivo de saída.\n");
        fclose(arquivo_entrada);
        return;
    }

    //todas as possibilidades de combinação
    int total_variaveis = numero_vertices * numero_cores;
    //a principio, não tem nenhuma clausula
    int total_clausulas = 0;

    //escreve o cabeçalho do arquivo CNF, mas sem o numero de clausulas pois ainda nao foram escritas
    fprintf(arquivo_saida, "p cnf %d 0\n", total_variaveis);

    // Condição 1: Cada vértice deve ter pelo menos uma cor 
    for (int vertice = 0; vertice < numero_vertices; vertice++) {//percorre todos os vertices
        for (int cor = 0; cor < numero_cores; cor++) {//e todas as cores para cada vertice
            fprintf(arquivo_saida, "%d ", nova_variavel(vertice, cor, numero_cores));
            //gera uma variavel para cada combinação
        }
        fprintf(arquivo_saida, "0\n");
        //finaliza a clausula após gerar as variáveis para um vértice
        total_clausulas++;
    }

    // Condição 2: Cada vértice não pode ter duas cores ao mesmo tempo
    for (int vertice = 0; vertice < numero_vertices; vertice++) {//percorre todos os vertices
        for (cor1 = 0; cor1 < numero_cores; cor1++) {//percorre a primeira cor possivel para esse vertice
            for (cor2 = cor1 + 1; cor2 < numero_cores; cor2++) { //percorre a segunda cor possivel para esse vertice
                //cor2 = cor1+1 para que as cores sejam sempre diferentes
                fprintf(arquivo_saida, "-%d -%d 0\n",
                //as variáveis são negadas pois um vertice pode não ter nenhuma das duas cores, mas não pode ter as duas ao mesmo tempo
                    nova_variavel(vertice, cor1, numero_cores),
                    nova_variavel(vertice, cor2, numero_cores));
                total_clausulas++;
            }
        }
    }

    // Condição 3: Vértices adjacentes não podem ter a mesma cor
    for (indice_aresta = 0; indice_aresta < numero_arestas; indice_aresta++) {//percorre as arestas
        //armazena os dois vertices da aresta
        vertice_u = lista_arestas[indice_aresta][0]; 
        vertice_v = lista_arestas[indice_aresta][1];
        for (int cor = 0; cor < numero_cores; cor++) {//percorre as cores possíveis
            fprintf(arquivo_saida, "-%d -%d 0\n",
                nova_variavel(vertice_u, cor, numero_cores),
                nova_variavel(vertice_v, cor, numero_cores));
            //gera uma clausula com uma variavel correspondente a cada par (vertice,cor)
            //negadas novamente pois os dois vertices podem não ter aquela cor, mas os dois não podem ter a mesma cor
            total_clausulas++;
        }
    }

    //atualiza o cabeçalho com o numero de clausulas
    rewind(arquivo_saida);
    fprintf(arquivo_saida, "p cnf %d %d\n", total_variaveis, total_clausulas);

    fclose(arquivo_entrada);
    fclose(arquivo_saida);

    printf("O arquivo CNF foi gerado com sucesso como 'saida.cnf'.\n");
}

//traduz o resultado SAT para pares (Vértice, Cor)
//a tradução é o processo inverso da geração de variáveis unicas
void traduzir_resultado() {
    FILE *arquivo_resultado;
    //variaveis necessárias para a tradução
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

/
// SAT solver

// --- Definições de estruturas ---
// NoArvore: representa cada nó de uma árvore binária implícita
typedef struct NoArvore {
    int indice_variavel; // índice da variável correspondente (0..n-1)
    struct NoArvore *esquerda; // ponteiro para subárvore com valor 1 nessa variável
    struct NoArvore *direita; // ponteiro para subárvore com valor 0 nessa variável
    //cada nó representa uma váriável da fórmula
} NoArvore;

// FormulaCNF: armazena a lista de cláusulas e parâmetros da fórmula
typedef struct {
    int **clausulas;// vetor de cláusulas, cada cláusula é vetor de literais terminado em 0
    //basicamente um vetor de ponteiros, que apontam para outros vetores
    int num_clausulas; // número de cláusulas lidas
    int num_variaveis;  // número de variáveis na fórmula (máximo literal absoluto)
} FormulaCNF;

// Cria árvore binária de profundidade 'num_variaveis'
// Cada nível corresponde a uma variável, ramificando em verdadeiro (esquerda) e falso (direita)
NoArvore *criar_arvore(int profundidade, int num_variaveis) {
    // Se já definimos todas as variáveis, não criamos mais nós
    if (profundidade >= num_variaveis) return NULL;
    // Aloca um novo nó e define seu índice de variável
    NoArvore *no = malloc(sizeof(*no));
    no->indice_variavel = profundidade;
    // Cria recursivamente as subárvores:
    // esquerda = atribuir valor 1 à variável atual
    no->esquerda = criar_arvore(profundidade + 1, num_variaveis);
    // direita  = atribuir valor 0 à variável atual
    no->direita  = criar_arvore(profundidade + 1, num_variaveis);
    return no;
}

// Libera memória da árvore 
// Percorre em pós-ordem para liberar subárvores antes do nó
void liberar_arvore(NoArvore *no) {
    if (!no) return;
    liberar_arvore(no->esquerda);
    liberar_arvore(no->direita);
    free(no);
}

// Verifica se a fórmula inteira está satisfeita 
// Para cada cláusula, testa se existe um literal que seja verdadeiro na atribuição atual
bool formula_satisfeita(const FormulaCNF *formula, int atribuicao[]) {
    for (int i = 0; i < formula->num_clausulas; i++) {
        int *clausula = formula->clausulas[i];
        bool clausula_satisfeita = false;

        // Percorre todos os literais até encontrar o marcador de fim (0)
        for (int j = 0; clausula[j] != 0; j++) {
            int literal = clausula[j];
            int indice_variavel = abs(literal) - 1; // Converte literal para índice 0-based
            
             //Verifica se a variável correspondente ao literal está atribuída como 1 (verdadeira).
            bool variavel_verdadeira =  false;
            if(atribuicao[indice_variavel] == 1)
            {
                variavel_verdadeira = true;
            }

            // Verifica se o literal está satisfeito:
            // - Literal positivo: variável deve ser 1
            // - Literal negativo: variável deve ser 0
            if ((literal > 0 && variavel_verdadeira) || (literal < 0 && !variavel_verdadeira)) {
                clausula_satisfeita = true;
                break; // Cláusula satisfeita, passa para a próxima
            }
        }
        // Se a cláusula atual não foi satisfeita, a fórmula é UNSAT
        if (!clausula_satisfeita) return false;
    }
    return true; // Todas as cláusulas foram satisfeitas
}

// Gera atribuições usando a árvore para recursão implícita 
// Caminha a árvore, atribuindo valor 1 (esquerda) antes de 0 (direita) em cada variável
bool gerar_com_arvore(NoArvore *no_atual, int atribuicao[], const FormulaCNF *formula, bool *solucao_encontrada) {
    // Interrompe apenas se já encontrou uma solução
    if (*solucao_encontrada) return true;
    
    // Se chegou a um nó nulo, todas as variáveis foram atribuídas
    if (no_atual == NULL) {
        // Verifica se a atribuição atual satisfaz a fórmula completa
        if (formula_satisfeita(formula, atribuicao)) {
            *solucao_encontrada = true;
        }
        return *solucao_encontrada;
    }
    
    int indice_variavel = no_atual->indice_variavel;

    // Explora subárvore esquerda (atribui valor 1 à variável atual)
    atribuicao[indice_variavel] = 1;
    gerar_com_arvore(no_atual->esquerda, atribuicao, formula, solucao_encontrada);
    if (*solucao_encontrada) return true;

    // Explora subárvore direita (atribui valor 0 à variável atual)
    atribuicao[indice_variavel] = 0;
    gerar_com_arvore(no_atual->direita, atribuicao, formula, solucao_encontrada);


    return *solucao_encontrada;
}

FormulaCNF *ler_arquivo_cnf(const char *nome_arquivo) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) { 
        perror("Erro ao abrir arquivo"); 
        return NULL; 
    }

    FormulaCNF *formula = malloc(sizeof(*formula));
    formula->num_clausulas = 0;
    formula->num_variaveis = 0;
    formula->clausulas = NULL;

    char linha[256];
    int capacidade_clausulas = 0; // Capacidade inicial do vetor de cláusulas
    int contador_clausulas = 0; // Número real de cláusulas lidas

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (linha[0] == 'c') continue; // Ignora linhas de comentário
        if (linha[0] == 'p') {
            // Lê o cabeçalho 'p cnf' com número de variáveis e cláusulas
            sscanf(linha, "p cnf %d %d", &formula->num_variaveis, &formula->num_clausulas);
            capacidade_clausulas = formula->num_clausulas;
            //aloca espaço para o vetor de clausulas, com capacidade inical  igual ao numero de clausulas
            formula->clausulas = malloc(sizeof(int*) * capacidade_clausulas);
            continue;
        }
        // Realoca o vetor de cláusulas se necessário
        if (contador_clausulas >= capacidade_clausulas) {
            capacidade_clausulas *= 2;
            formula->clausulas = realloc(formula->clausulas, sizeof(int*) * capacidade_clausulas);
        }
        
       // Buffer para armazenar os literais da cláusula atual
        int *literais_clausula = NULL;
        int tamanho_literais = 0;    // Número de literais lidos
        int capacidade_literais = 0; // Capacidade atual do buffer

        // Divide a linha em tokens (literais)
        char *token = strtok(linha, " \n");
        while (token != NULL) {
            //converte o token de string para inteiro
            int literal = atoi(token);
            if (literal == 0) break; // Fim da cláusula

            // Validação do literal
            //se o literal for um numero maior que o numero de variaveis, retorna erro
            if (abs(literal) > formula->num_variaveis) {
                fprintf(stderr, "Erro: Literal %d fora do intervalo\n", literal);
                fclose(arquivo);
                free(literais_clausula); // Libera o buffer antes de retornar
                free(formula->clausulas);
                free(formula);
                return NULL;
            }

            // Expande o buffer de literais se necessário
            if (tamanho_literais >= capacidade_literais) {
                //Se a capacidade ainda não foi definida (ou seja, 0), ela é inicializada com 4. Caso contrário, ela dobra de tamanho
                capacidade_literais = (capacidade_literais == 0) ? 4 : capacidade_literais * 2;
                //realoca memoria para armazenar mais literais, sem perder os já lidos
                literais_clausula = realloc(literais_clausula, sizeof(int) * capacidade_literais);
            }
            //armazena o valor de litera, no vetor, literais_clausula
            //usa o indice atual de tamanho_literais, delposi incrementa ele
            literais_clausula[tamanho_literais++] = literal;
            //chama o proximo token da string que está sendo lida
            token = strtok(NULL, " \n");
        }

        // Verifica se a clausula não está vazia, para adicioná-la a formula
        if (tamanho_literais > 0) {
            // Redimensiona o vetor literais_clausula para comportar um elemento extra, que será usado como marcador de fim (o 0 no formato CNF)
            literais_clausula = realloc(literais_clausula, sizeof(int) * (tamanho_literais + 1));
            //adiciona o terminador 0 na ultima posição do vetor
            literais_clausula[tamanho_literais] = 0; // Terminador
            //Adiciona o ponteiro para o vetor de literais da cláusula no vetor clausulas da fórmula
            //Atualiza o contador de cláusulas
            formula->clausulas[contador_clausulas++] = literais_clausula;
        }
    }

    fclose(arquivo);
    formula->num_clausulas = contador_clausulas; // Atualiza com o número real
    return formula;
}

// Libera memória da fórmula 
// Libera cada cláusula e o vetor de ponteiros, depois a própria estrutura
void liberar_formula(FormulaCNF *formula) {
    for (int i = 0; i < formula->num_clausulas; i++){
        free(formula->clausulas[i]);
    }
    free(formula->clausulas);
    free(formula);
}

void resolver_sat() {
    const char *nome_arquivo = "saida.cnf";
    FormulaCNF *formula = ler_arquivo_cnf(nome_arquivo);
    if (!formula) return;

    // Caso SAT trivial: fórmula sem cláusulas é sempre satisfatível
    if (formula->num_clausulas == 0) {
        printf("SAT\n");
        liberar_formula(formula);
        return;
    }

    // Cria um vetor para armazenar a atribuição booleana de cada variável.
    //Valor -1 indica que a variável ainda não foi atribuída.
    int *atribuicoes = malloc(sizeof(int) * formula->num_variaveis);
    for (int i = 0; i < formula->num_variaveis; i++) atribuicoes[i] = -1;

    // Cria árvore de decisão para explorar todas as combinações possíveis
    NoArvore *arvore = criar_arvore(0, formula->num_variaveis);
    bool solucao_encontrada = false;

    // Realiza busca exaustiva usando a árvore
    gerar_com_arvore(arvore, atribuicoes, formula, &solucao_encontrada);
    liberar_arvore(arvore);

   //Escreve resultados no arquivo de saída
    FILE *saida = fopen("resultado_sat.txt", "w");
    if (solucao_encontrada) {
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
