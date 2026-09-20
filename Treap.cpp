#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <filesystem>
#include "Arvores.h"
using namespace std;

using Relogio = chrono::high_resolution_clock;
const char* ARQUIVO_RESULTADOS = "resultados/resultados.csv";

class noTreap{
    public:
        int chave, prioridade;
        struct noTreap *esquerda, *direita;
        noTreap(int c, int p) : chave(c), prioridade(p), esquerda(nullptr), direita(nullptr) {}
};
class Treap {
private:
    noTreap* raiz;
 
    // Gerador de prioridades aleatórias (é isso que garante o
    // balanceamento probabilístico: se as prioridades forem uniformes,
    // a árvore resultante tem altura esperada O(log n), igual a uma
    // BST construída com inserções em ordem aleatória)
    mt19937 rng;
    uniform_int_distribution<int> distPrioridade;
 
    // ---------- Rotações (idênticas às da AVL) ----------
    // Antes:        y                Depois:      x
    //              / \                           / |
    //             x   T3      rotDireita(y)     T1  y
    //            / \        <----------------      / |
    //           T1  T2                            T2 T3
    noTreap* rotacaoDireita(noTreap* y) {
        noTreap* x = y->esquerda;
        noTreap* T2 = x->direita;
 
        x->direita = y;
        y->esquerda = T2;
 
        return x; // x assume o lugar de y
    }
 
    // Antes:      x                  Depois:        y
    //            / \                               / |
    //           T1  y      rotEsquerda(x)          x  T3
    //              / \    ---------------->        / |
    //             T2 T3                            T1 T2
    noTreap* rotacaoEsquerda(noTreap* x) {
        noTreap* y = x->direita;
        noTreap* T2 = y->esquerda;
 
        y->esquerda = x;
        x->direita = T2;
 
        return y; // y assume o lugar de x
    }
 
    // ---------- Inserção ----------
    // 1) Insere como numa BST comum, pela chave.
    // 2) Na volta da recursão, se o filho tiver prioridade MAIOR que a
    //    do pai, o heap foi violado -> rotaciona pra consertar.
    noTreap* inserir(noTreap* no, int chave) {
        if (no == nullptr) {
            return new noTreap(chave, distPrioridade(rng));
        }
 
        if (chave < no->chave) {
            no->esquerda = inserir(no->esquerda, chave);
            // violou o heap à esquerda?
            if (no->esquerda->prioridade > no->prioridade) {
                no = rotacaoDireita(no);
            }
        } else if (chave > no->chave) {
            no->direita = inserir(no->direita, chave);
            // violou o heap à direita?
            if (no->direita->prioridade > no->prioridade) {
                no = rotacaoEsquerda(no);
            }
        }
        // chave repetida: não insere (trate como quiser: contador, ignorar, etc.)
 
        return no;
    }
 
    // ---------- Busca ----------
    // É busca de BST pura: a prioridade não interfere na busca,
    // só na forma como a árvore ficou balanceada.
    bool buscar(noTreap* no, int chave) {
        if (no == nullptr) return false;
        if (chave == no->chave) return true;
        if (chave < no->chave) return buscar(no->esquerda, chave);
        return buscar(no->direita, chave);
    }
 
    // ---------- Remoção ----------
    // Ideia: em vez de achar sucessor/antecessor como numa BST comum,
    // "empurra" o nó pra baixo rotacionando sempre para o lado do
    // filho de MAIOR prioridade, até ele virar folha (ou ter só um
    // filho), e então remove diretamente.
    noTreap* remover(noTreap* no, int chave) {
        if (no == nullptr) return nullptr;
 
        if (chave < no->chave) {
            no->esquerda = remover(no->esquerda, chave);
        } else if (chave > no->chave) {
            no->direita = remover(no->direita, chave);
        } else {
            // achou o nó a remover
            if (no->esquerda == nullptr) {
                noTreap* temp = no->direita;
                delete no;
                return temp;
            } else if (no->direita == nullptr) {
                noTreap* temp = no->esquerda;
                delete no;
                return temp;
            } else {
                // tem os dois filhos: rotaciona na direção do filho
                // de maior prioridade para empurrar 'no' para baixo
                if (no->esquerda->prioridade > no->direita->prioridade) {
                    no = rotacaoDireita(no);
                    no->direita = remover(no->direita, chave);
                } else {
                    no = rotacaoEsquerda(no);
                    no->esquerda = remover(no->esquerda, chave);
                }
            }
        }
        return no;
    }
 
    // ---------- Percurso in-order (mostra a BST por chave) ----------
    void emOrdem(noTreap* no) {
        if (no == nullptr) return;
        emOrdem(no->esquerda);
        cout << no->chave << " ";
        emOrdem(no->direita);
    }
 
    // ---------- Impressão estrutural (mostra chave/prioridade e forma) ----------
    // Útil pra gerar os "snapshots" que o relatório pede na Seção 3.
    void imprimirEstrutura(noTreap* no, int nivel = 0) {
        if (no == nullptr) return;
        imprimirEstrutura(no->direita, nivel + 1);
        cout << string(nivel * 4, ' ')
             << no->chave << "(p=" << no->prioridade << ")" << endl;
        imprimirEstrutura(no->esquerda, nivel + 1);
    }
 
    void destruir(noTreap* no) {
        if (no == nullptr) return;
        destruir(no->esquerda);
        destruir(no->direita);
        delete no;
    }

    // gera as linhas do arquivo .dot -- rotula cada no com "chave (p=prioridade)"
    // pra deixar visivel, na propria imagem, o porque das rotacoes (a
    // propriedade de heap sobre as prioridades e o que define o formato).
    int exportarDot(noTreap* no, ofstream &arquivo, int &contador) {
        if (no == nullptr) return -1;
        int idAtual = contador++;
        arquivo << "    n" << idAtual << " [label=\"" << no->chave
                << "\\n(p=" << no->prioridade << ")\"];\n";

        int idEsquerda = exportarDot(no->esquerda, arquivo, contador);
        if (idEsquerda != -1) arquivo << "    n" << idAtual << " -> n" << idEsquerda << " [label=\"E\"];\n";

        int idDireita = exportarDot(no->direita, arquivo, contador);
        if (idDireita != -1) arquivo << "    n" << idAtual << " -> n" << idDireita << " [label=\"D\"];\n";

        return idAtual;
    }
 
public:
    Treap(int seed = 42, int prioridadeMax = 1000)
        : raiz(nullptr), rng(seed), distPrioridade(1, prioridadeMax) {}
 
    ~Treap() { destruir(raiz); }
 
    void inserir(int chave) { raiz = inserir(raiz, chave); }
    void remover(int chave) { raiz = remover(raiz, chave); }
    bool buscar(int chave) { return buscar(raiz, chave); }
 
    void imprimirEmOrdem() {
        emOrdem(raiz);
        cout << endl;
    }
 
    void imprimirEstrutura() {
        cout << "----- estrutura da treap -----" << endl;
        imprimirEstrutura(raiz);
        cout << "-------------------------------" << endl;
    }

    // Exporta a arvore atual para um arquivo .dot (formato Graphviz),
    // rotulado com "titulo" -- deixa explicito, no proprio arquivo, de
    // qual estrutura e estado se trata. Renderize com:
    //   dot -Tpng caminho.dot -o caminho.png
    void exportarDot(const string &caminho, const string &titulo) {
        filesystem::path caminhoPath(caminho);
        if (caminhoPath.has_parent_path()) {
            filesystem::create_directories(caminhoPath.parent_path());
        }

        ofstream arquivo(caminho);
        if (!arquivo.is_open()) {
            cerr << "Aviso: nao foi possivel criar \"" << caminho << "\".\n";
            return;
        }
        arquivo << "// Arvore Treap -- gerado automaticamente\n";
        arquivo << "digraph TreapTree {\n";
        arquivo << "    label=\"" << titulo << "\";\n";
        arquivo << "    labelloc=t;\n";
        arquivo << "    node [shape=circle, style=filled, fillcolor=\"#d7f5d0\"];\n";
        if (raiz != nullptr) {
            int contador = 0;
            exportarDot(raiz, arquivo, contador);
        } else {
            arquivo << "    vazio [label=\"(arvore vazia)\", shape=plaintext];\n";
        }
        arquivo << "}\n";
        cout << "Arvore exportada para \"" << caminho << "\" (renderize com: dot -Tpng "
             << caminho << " -o " << caminho.substr(0, caminho.find_last_of('.')) << ".png)\n";
    }
};
 
int main(int argc, char* argv[]) {
    // "input/dados_treap.dat" e relativo ao diretorio de onde o executavel
    // e rodado -- execute a partir da pasta EmCpp (onde fica a pasta input/).
    // Opcionalmente aceita o caminho do arquivo como argumento, pra rodar
    // com os tamanhos gerados em input/gerados/ sem precisar recompilar:
    //   ./build/Treap input/gerados/dados_treap_10000.dat
    string caminhoDados = (argc > 1) ? argv[1] : "input/dados_treap.dat";
    vector<Secao> secoes = lerSecoes(caminhoDados);

    // Cada bloco (RANDOM, SORTED, REVERSE) e testado numa treap propria,
    // pra comparar o efeito da ORDEM de insercao -- e justamente isso que
    // o arquivo quer demonstrar (SORTED/REVERSE seriam o pior caso para
    // uma BST comum, mas a treap se auto-balanceia via prioridade).
    for (const Secao &secao : secoes) {
        vector<int> chaves = paraInteiros(secao.dados);

        // seed fixa (42) só pra reprodutibilidade nos testes/relatório;
        // pra experimentos "reais" no benchmark, use uma seed variável
        // ou random_device.
        Treap treap(42);

        cout << "\n=== Bloco " << secao.rotulo << " (" << chaves.size() << " chaves) ===\n";

        auto inicioInsercao = Relogio::now();
        for (int c : chaves) treap.inserir(c);
        auto fimInsercao = Relogio::now();

        double tempoInsercaoMs = chrono::duration<double, milli>(fimInsercao - inicioInsercao).count();
        cout << "  tempo de insercao: " << tempoInsercaoMs << " ms\n";
        gravarTempo(ARQUIVO_RESULTADOS, "Treap", secao.rotulo, "insercao", (long long)chaves.size(), tempoInsercaoMs);

        // busca todas as chaves inseridas (uma leva "busca com sucesso" em
        // massa, pra medir tempo de forma mais estavel do que 1 busca so)
        auto inicioBusca = Relogio::now();
        int encontrados = 0;
        for (int c : chaves) if (treap.buscar(c)) encontrados++;
        auto fimBusca = Relogio::now();

        double tempoBuscaMs = chrono::duration<double, milli>(fimBusca - inicioBusca).count();
        cout << "  tempo de busca (" << chaves.size() << " buscas, " << encontrados << " encontradas): " << tempoBuscaMs << " ms\n";
        gravarTempo(ARQUIVO_RESULTADOS, "Treap", secao.rotulo, "busca", (long long)chaves.size(), tempoBuscaMs);

        cout << "Buscando um valor que nao existe (-1): "
             << (treap.buscar(-1) ? "encontrado" : "nao encontrado") << "\n";

        // remove metade das chaves, pra ter tambem uma medicao/demonstracao
        // de remocao (Secao 3 e 5 do relatorio)
        size_t metade = chaves.size() / 2;
        auto inicioRemocao = Relogio::now();
        for (size_t i = 0; i < metade; i++) treap.remover(chaves[i]);
        auto fimRemocao = Relogio::now();

        double tempoRemocaoMs = chrono::duration<double, milli>(fimRemocao - inicioRemocao).count();
        cout << "  tempo de remocao (" << metade << " chaves): " << tempoRemocaoMs << " ms\n";
        gravarTempo(ARQUIVO_RESULTADOS, "Treap", secao.rotulo, "remocao", (long long)metade, tempoRemocaoMs);
    }

    // ---------- Demonstracao pequena, exportada em .dot (Secao 3) ----------
    // As remocoes acima sao em treaps de centenas de chaves -- otimas pra
    // medir tempo, mas ilegiveis como "evidencia visual" no relatorio.
    // Aqui, uma treap pequena so pra mostrar o antes/depois de uma remocao
    // especifica (inclusive as rotacoes que a remocao provoca pra "descer"
    // o no ate virar folha) -- exportada direto como imagem, sem imprimir
    // nada no terminal (renderize com: dot -Tpng arquivo.dot -o saida.png).
    cout << "\n\n===== Demonstracao de remocao (exemplo pequeno p/ Secao 3) =====\n";
    Treap exemplo(42); // mesma seed fixa dos blocos acima, so por consistencia
    vector<int> chavesExemplo = {50, 30, 70, 20, 40, 60, 80, 10, 90, 35};
    for (int c : chavesExemplo) exemplo.inserir(c);

    exemplo.exportarDot("resultados/diagramas/treap_antes.dot", "Treap - ANTES de remover a chave 40");

    exemplo.remover(40);

    exemplo.exportarDot("resultados/diagramas/treap_depois.dot", "Treap - DEPOIS de remover a chave 40");

    return 0;
}