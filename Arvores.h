#ifndef ARVORES_H
#define ARVORES_H
using namespace std;

#include <string>
#include <vector>
#include <utility>

// Normaliza uma palavra: converte para minusculas, troca letras acentuadas
// do portugues pela letra "base" (a, e, i, o, u, c, n) e descarta qualquer
// outro caractere (numeros, espacos, pontuacao, simbolos).
// Assume entrada em UTF-8 (padrao do sistema/terminal).
string normalizar(const string &entrada);

// ---------------------------------------------------------------------
// Leitura padronizada dos arquivos de entrada (pasta input/)
// ---------------------------------------------------------------------
//
// Os arquivos de splay, treap e kdtree seguem o mesmo formato, em blocos:
//
//   # comentario explicando o bloco (ignorado)
//   ROTULO quantidade
//   dado_1
//   dado_2
//   ...
//   dado_quantidade
//
// podendo ter varios blocos em sequencia (ex.: INSERT, depois ACCESS,
// depois HOTSET). Uma Secao representa um desses blocos ja separado,
// mas ainda com os dados em texto puro (cada arvore converte para o
// tipo que precisa: inteiros ou pontos).
struct Secao {
    string rotulo;         // ex: "INSERT", "RANDOM", "UNIFORME", "QUERIES"...
    vector<string> dados;  // uma linha de dado por elemento, ja sem '\r'/'\n'
};

// Le todas as secoes de um arquivo no formato acima (splay, treap, kdtree).
vector<Secao> lerSecoes(const string &caminho);

// Converte as linhas de uma secao (ou qualquer vetor de strings) em inteiros.
// Usado por splay e treap, cujas chaves sao int.
vector<int> paraInteiros(const vector<string> &linhas);

// Converte as linhas de uma secao em pontos 2D "x y" (usado pela kdtree).
vector<pair<double, double>> paraPontos(const vector<string> &linhas);

// Converte as linhas de uma secao em pontos de dimensao arbitraria: cada
// linha pode ter "x y", "x y z", "x y z w", etc. -- a dimensao e' inferida
// pela quantidade de numeros na PRIMEIRA linha nao vazia, e as demais linhas
// devem seguir a mesma dimensao (usado pela kdtree generalizada para n-D).
vector<vector<double>> paraPontosND(const vector<string> &linhas);

// Le uma lista simples de palavras, uma por linha (sem blocos/rotulos),
// ja normalizadas. Usado por trie e patricia.
vector<string> lerPalavras(const string &caminho);

// ---------------------------------------------------------------------
// Medicao de tempo (Secao 5 do relatorio: metodologia experimental)
// ---------------------------------------------------------------------
//
// gravarTempo() grava (em modo append) uma linha de resultado no arquivo
// CSV indicado. Se o arquivo ainda nao existir, cria com cabecalho antes.
// Chame uma vez por bloco/operacao medida -- os resultados de TODAS as
// estruturas podem ir pro mesmo CSV (ex.: "resultados.csv"), o que facilita
// montar os graficos comparativos da Secao 5 depois (ex. em Python/pandas).
//
// Parametros:
//   arquivoCSV : caminho do CSV (ex.: "resultados/resultados.csv")
//   estrutura  : nome da estrutura (ex.: "Splay", "Treap", "Trie",
//                "Patricia", "KDTree")
//   bloco      : nome do bloco/cenario de dados (ex.: "INSERT", "SORTED",
//                "UNIFORME", "palavras")
//   operacao   : nome da operacao medida (ex.: "insercao", "busca",
//                "remocao", "construcao")
//   quantidade : quantos elementos/operacoes foram executados na medicao
//   tempoMs    : tempo total gasto, em milissegundos
void gravarTempo(const string &arquivoCSV, const string &estrutura,
                  const string &bloco, const string &operacao,
                  long long quantidade, double tempoMs);

#endif