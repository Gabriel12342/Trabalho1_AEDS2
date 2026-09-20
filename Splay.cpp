#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include "Arvores.h"
using namespace std;

// alias curto pro relogio de alta resolucao usado nas medicoes de tempo
using Relogio = chrono::high_resolution_clock;
const char* ARQUIVO_RESULTADOS = "resultados/resultados.csv";

// ---------- Nó da Splay ----------
struct NoSplay {
    int chave;
    NoSplay *esquerda, *direita;

    NoSplay(int c) : chave(c), esquerda(nullptr), direita(nullptr) {}
};

class Splay {
private:
    NoSplay* raiz;

    // ---------- Rotações (as mesmas de sempre) ----------
    NoSplay* rotacaoDireita(NoSplay* y) {
        NoSplay* x = y->esquerda;
        y->esquerda = x->direita;
        x->direita = y;
        return x;
    }

    NoSplay* rotacaoEsquerda(NoSplay* x) {
        NoSplay* y = x->direita;
        x->direita = y->esquerda;
        y->esquerda = x;
        return y;
    }

    // ---------- Splay ----------
    // Traz o nó de chave 'chave' para a raiz. Se a chave não existir,
    // traz o último nó visitado no caminho de busca (o mais "próximo"
    // dela) -- é esse detalhe que faz até buscas malsucedidas
    // reorganizarem a árvore de forma útil.
    NoSplay* splay(NoSplay* raizAtual, int chave) {
        if (raizAtual == nullptr || raizAtual->chave == chave) {
            return raizAtual;
        }

        if (chave < raizAtual->chave) {
            if (raizAtual->esquerda == nullptr) return raizAtual; // não achou, para aqui

            if (chave < raizAtual->esquerda->chave) {
                // caso esquerda-esquerda (zig-zig)
                raizAtual->esquerda->esquerda = splay(raizAtual->esquerda->esquerda, chave);
                raizAtual = rotacaoDireita(raizAtual);
            } else if (chave > raizAtual->esquerda->chave) {
                // caso esquerda-direita (zig-zag)
                raizAtual->esquerda->direita = splay(raizAtual->esquerda->direita, chave);
                if (raizAtual->esquerda->direita != nullptr) {
                    raizAtual->esquerda = rotacaoEsquerda(raizAtual->esquerda);
                }
            }

            // zig: sobe o filho esquerdo uma vez (cobre o caso base
            // e finaliza os dois casos acima)
            return (raizAtual->esquerda == nullptr) ? raizAtual : rotacaoDireita(raizAtual);

        } else { // chave > raizAtual->chave
            if (raizAtual->direita == nullptr) return raizAtual;

            if (chave > raizAtual->direita->chave) {
                // caso direita-direita (zig-zig)
                raizAtual->direita->direita = splay(raizAtual->direita->direita, chave);
                raizAtual = rotacaoEsquerda(raizAtual);
            } else if (chave < raizAtual->direita->chave) {
                // caso direita-esquerda (zig-zag)
                raizAtual->direita->esquerda = splay(raizAtual->direita->esquerda, chave);
                if (raizAtual->direita->esquerda != nullptr) {
                    raizAtual->direita = rotacaoDireita(raizAtual->direita);
                }
            }

            return (raizAtual->direita == nullptr) ? raizAtual : rotacaoEsquerda(raizAtual);
        }
    }

    void emOrdem(NoSplay* no) {
        if (no == nullptr) return;
        emOrdem(no->esquerda);
        cout << no->chave << " ";
        emOrdem(no->direita);
    }

    void imprimirEstrutura(NoSplay* no, int nivel = 0) {
        if (no == nullptr) return;
        imprimirEstrutura(no->direita, nivel + 1);
        cout << string(nivel * 4, ' ') << no->chave << endl;
        imprimirEstrutura(no->esquerda, nivel + 1);
    }

    // gera as linhas "idPai -> idFilho" do arquivo .dot, percorrendo a
    // arvore. Cada no recebe um id proprio (ex.: n0, n1, n2...) porque
    // chaves repetidas ou nos diferentes com o mesmo valor nao poderiam
    // ser distinguidos so pelo rotulo.
    int exportarDot(NoSplay* no, ofstream &arquivo, int &contador) {
        if (no == nullptr) return -1;
        int idAtual = contador++;
        arquivo << "    n" << idAtual << " [label=\"" << no->chave << "\"];\n";

        int idEsquerda = exportarDot(no->esquerda, arquivo, contador);
        if (idEsquerda != -1) arquivo << "    n" << idAtual << " -> n" << idEsquerda << " [label=\"E\"];\n";

        int idDireita = exportarDot(no->direita, arquivo, contador);
        if (idDireita != -1) arquivo << "    n" << idAtual << " -> n" << idDireita << " [label=\"D\"];\n";

        return idAtual;
    }

    void destruir(NoSplay* no) {
        if (no == nullptr) return;
        destruir(no->esquerda);
        destruir(no->direita);
        delete no;
    }

public:
    Splay() : raiz(nullptr) {}
    ~Splay() { destruir(raiz); }

    // ---------- Busca ----------
    // Sempre splaya, independente de achar ou não -- essa é a
    // essência do autoajuste: quanto mais um elemento (ou algo perto
    // dele) é consultado, mais perto da raiz ele fica.
    bool buscar(int chave) {
        raiz = splay(raiz, chave);
        return raiz != nullptr && raiz->chave == chave;
    }

    // ---------- Inserção ----------
    void inserir(int chave) {
        if (raiz == nullptr) {
            raiz = new NoSplay(chave);
            return;
        }

        raiz = splay(raiz, chave);

        if (raiz->chave == chave) return; // já existe, não duplica

        NoSplay* novo = new NoSplay(chave);
        if (chave < raiz->chave) {
            novo->direita = raiz;
            novo->esquerda = raiz->esquerda;
            raiz->esquerda = nullptr;
        } else {
            novo->esquerda = raiz;
            novo->direita = raiz->direita;
            raiz->direita = nullptr;
        }
        raiz = novo;
    }

    // ---------- Remoção ----------
    // Splaya o nó a remover até a raiz -> a árvore vira duas
    // subárvores separadas (esquerda e direita) -> "gruda" as duas
    // splayando o maior elemento da esquerda até virar a nova raiz
    // (assim ele fica sem filho direito, pronto pra receber a
    // subárvore direita).
    void remover(int chave) {
        if (raiz == nullptr) return;

        raiz = splay(raiz, chave);
        if (raiz->chave != chave) return; // não existe

        NoSplay* esquerda = raiz->esquerda;
        NoSplay* direita = raiz->direita;
        delete raiz;

        if (esquerda == nullptr) {
            raiz = direita;
        } else {
            esquerda = splay(esquerda, chave); // traz o maior da esquerda pro topo
            esquerda->direita = direita;
            raiz = esquerda;
        }
    }

    void imprimirEmOrdem() {
        emOrdem(raiz);
        cout << endl;
    }

    void imprimirEstrutura() {
        cout << "----- estrutura da splay (raiz = ultimo acessado) -----" << endl;
        imprimirEstrutura(raiz);
        cout << "---------------------------------------------------------" << endl;
    }

    // Exporta a arvore atual para um arquivo .dot (formato Graphviz).
    // "titulo" aparece como rotulo do grafico inteiro, deixando explicito
    // no proprio arquivo de qual estrutura e estado se trata -- depois e
    // so rodar:  dot -Tpng caminho.dot -o caminho.png
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
        arquivo << "// Arvore Splay -- gerado automaticamente\n";
        arquivo << "digraph SplayTree {\n";
        arquivo << "    label=\"" << titulo << "\";\n";
        arquivo << "    labelloc=t;\n";
        arquivo << "    node [shape=circle, style=filled, fillcolor=\"#cfe8ff\"];\n";
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
    Splay arvore;

    // "input/dados_splay.dat" e relativo ao diretorio de onde o executavel
    // e rodado -- execute a partir da pasta EmCpp (onde fica a pasta input/).
    // Opcionalmente aceita o caminho do arquivo como argumento, pra rodar
    // com os tamanhos gerados em input/gerados/ sem precisar recompilar:
    //   ./build/Splay input/gerados/dados_splay_10000.dat
    string caminhoDados = (argc > 1) ? argv[1] : "input/dados_splay.dat";
    vector<Secao> secoes = lerSecoes(caminhoDados);

    for (const Secao &secao : secoes) {
        vector<int> chaves = paraInteiros(secao.dados);

        if (secao.rotulo == "INSERT") {
            cout << "Inserindo " << chaves.size() << " chaves (bloco INSERT)...\n";

            auto inicio = Relogio::now();
            for (int c : chaves) arvore.inserir(c);
            auto fim = Relogio::now();

            double tempoMs = chrono::duration<double, milli>(fim - inicio).count();
            cout << "  tempo: " << tempoMs << " ms\n";
            gravarTempo(ARQUIVO_RESULTADOS, "Splay", "INSERT", "insercao", (long long)chaves.size(), tempoMs);

        } else if (secao.rotulo == "ACCESS") {
            // e aqui que o efeito do splaying aparece: cada busca traz a
            // chave acessada (ou a mais proxima dela) para perto da raiz.
            cout << "Executando " << chaves.size() << " acessos (bloco ACCESS)...\n";
            int encontrados = 0;

            auto inicio = Relogio::now();
            for (int c : chaves) {
                if (arvore.buscar(c)) encontrados++;
            }
            auto fim = Relogio::now();

            double tempoMs = chrono::duration<double, milli>(fim - inicio).count();
            cout << "  tempo: " << tempoMs << " ms -- encontrados: " << encontrados << " de " << chaves.size() << "\n";
            gravarTempo(ARQUIVO_RESULTADOS, "Splay", "ACCESS", "busca", (long long)chaves.size(), tempoMs);

        } else if (secao.rotulo == "HOTSET") {
            // conjunto de chaves "quentes" usado so como referencia/plot no
            // relatorio -- nao precisa fazer nada com a arvore aqui.
            cout << "Conjunto quente (HOTSET) com " << chaves.size() << " chaves carregado.\n";
        }
    }

    cout << "\nArvore completa montada (impressao em texto desativada; use exportarDot() se quiser inspecionar).\n";

    // ---------- Demonstracao + medicao de tempo da remocao ----------
    // Remove as chaves do bloco HOTSET (as mesmas usadas no ACCESS) so
    // pra ter uma medicao de remocao tambem -- util pra Secao 5 (tempo)
    // do relatorio. A evidencia visual (Secao 3) fica por conta do
    // exportarDot() da demonstracao pequena, logo abaixo.
    vector<int> hotset;
    for (const Secao &secao : secoes) {
        if (secao.rotulo == "HOTSET") hotset = paraInteiros(secao.dados);
    }

    if (!hotset.empty()) {
        cout << "\nRemovendo as " << hotset.size() << " chaves do HOTSET...\n";

        auto inicio = Relogio::now();
        for (int c : hotset) arvore.remover(c);
        auto fim = Relogio::now();

        double tempoMs = chrono::duration<double, milli>(fim - inicio).count();
        cout << "  tempo: " << tempoMs << " ms\n";
        gravarTempo(ARQUIVO_RESULTADOS, "Splay", "HOTSET", "remocao", (long long)hotset.size(), tempoMs);
    }

    // ---------- Demonstracao pequena, exportada em .dot (Secao 3) ----------
    // As remocoes acima sao em arvores de centenas/milhares de chaves --
    // otimas pra medir tempo, mas ilegiveis como "evidencia visual" no
    // relatorio. Aqui, uma arvore pequena so pra mostrar o antes/depois
    // de uma remocao especifica -- exportada como imagem, sem imprimir
    // nada no terminal (renderize com: dot -Tpng arquivo.dot -o saida.png).
    cout << "\n\n===== Demonstracao de remocao (exemplo pequeno p/ Secao 3) =====\n";
    Splay exemplo;
    vector<int> chavesExemplo = {50, 30, 70, 20, 40, 60, 80, 10, 90, 35};
    for (int c : chavesExemplo) exemplo.inserir(c);

    exemplo.exportarDot("resultados/diagramas/splay_antes.dot", "Splay - ANTES de remover a chave 40");

    exemplo.remover(40);

    exemplo.exportarDot("resultados/diagramas/splay_depois.dot", "Splay - DEPOIS de remover a chave 40");

    return 0;
}