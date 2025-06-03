# GCP_with_SATSolver
Implementação do problema de coloração de vértices, com SAT Solver em C.

## Como usar? 
1 - O arquivo a ser executado é o "implementacao.c" (os arquivos "gcp_ok.c" e "sat_solver.c" são este mesmo arquivo separado em duas partes);  
2 - No arquivo entrada.txt, digite o Graph Coloring Problem seguindo a estrutura abaixo;  
  
    GCP <numero_de_vertices> <numero_de_cores> <numero_de _arestas> (na linha seguinte vamos inserir as arestas);    
    a <vertice_x1> <vertice_y1>  
    a <vertice_x2> <vertice_y2>  
    ...  
    a <vertice_xn> <vertice_yn>  
  
3 - Após isso, compilar e executar o programa normalmente;  
4 - Selecionar a opção 1 do progama ("Converter arquivo GCP para CNF");  
5 - Selecionar a opção 2 do programa ("Resolver problema SAT");  
6 - Para visualizar o resultado, ou seja, qual cor terá cada vértice, basta selecionar a opção 3 no menu do programa ("Traduzir resultado SAT");
    
