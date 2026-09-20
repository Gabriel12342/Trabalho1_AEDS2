#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
#include <fstream>
#include <filesystem>
#include "Arvores.h"
using namespace std;

using Relogio = chrono::high_resolution_clock;
const char* ARQUIVO_RESULTADOS = "resultados/resultados.csv";

// ---------- Ponto ----------
// Agora com dimensao arbitraria (antes era fixo em 2, so x e y).
// coord.size() == a dimensao k daquele ponto especifico; a arvore como um
// todo assume que TODOS os pontos inseridos tem a mesma dimensao k.
struct Ponto {
    vector<double> coord;

    Ponto() {}
    Ponto(vector<double> c) : coord(move(c)) {}

    size_t dimensao() const { return coord.size(); }
};

// ---------- Nó da KD-Tree ----------
struct NoKD {
    Ponto ponto;
    NoKD* esquerda;
    NoKD* direita;

    NoKD(Ponto p) : ponto(move(p)), esquerda(nullptr), direita(nullptr) {}
};

class KDTree {
private:
    NoKD* raiz;
    int k; // dimensao da arvore -- definida na construcao, a partir dos dados

    double distanciaQuadrada(const Ponto& a, const Ponto& b) {
        double soma = 0;
        for (int i = 0; i < k; i++) {
            double d = a.coord[i] - b.coord[i];
            soma += d * d;
        }
        return soma; // evita sqrt() enquanto so se quer COMPARAR distancias
    }

    // ---------- Inserção ----------
    // 'profundidade' decide o eixo de comparação: profundidade % k.
    // Em profundidade 0 compara a coordenada 0, em 1 compara a coordenada 1,
    // ..., em k volta pra coordenada 0, e assim por diante (generalizacao
    // direta do caso 2D, que so alternava entre x e y).
    NoKD* inserir(NoKD* no, const Ponto& p, int profundidade) {
        if (no == nullptr) {
            return new NoKD(p);
        }

        int eixo = profundidade % k;

        if (p.coord[eixo] < no->ponto.coord[eixo]) {
            no->esquerda = inserir(no->esquerda, p, profundidade + 1);
        } else {
            no->direita = inserir(no->direita, p, profundidade + 1);
        }

        return no;
    }

    // ---------- Busca exata ----------
    bool buscar(NoKD* no, const Ponto& p, int profundidade) {
        if (no == nullptr) return false;

        if (distanciaQuadrada(no->ponto, p) == 0) return true;

        int eixo = profundidade % k;
        if (p.coord[eixo] < no->ponto.coord[eixo]) {
            return buscar(no->esquerda, p, profundidade + 1);
        } else {
            return buscar(no->direita, p, profundidade + 1);
        }
    }

    // ---------- Vizinho mais próximo ----------
    // Mesma ideia de antes (poda por hiperplano), so que agora o "corte" em
    // cada nivel e' num dos k eixos em vez de so x/y.
    void vizinhoMaisProximo(NoKD* no, const Ponto& alvo, int profundidade,
                             NoKD*& melhor, double& melhorDist) {
        if (no == nullptr) return;

        double dist = distanciaQuadrada(no->ponto, alvo);
        if (dist < melhorDist) {
            melhorDist = dist;
            melhor = no;
        }

        int eixo = profundidade % k;
        double diferenca = alvo.coord[eixo] - no->ponto.coord[eixo];

        NoKD* ladoProvavel = (diferenca < 0) ? no->esquerda : no->direita;
        NoKD* ladoOposto   = (diferenca < 0) ? no->direita  : no->esquerda;

        vizinhoMaisProximo(ladoProvavel, alvo, profundidade + 1, melhor, melhorDist);

        if (diferenca * diferenca < melhorDist) {
            vizinhoMaisProximo(ladoOposto, alvo, profundidade + 1, melhor, melhorDist);
        }
    }

    // ---------- Impressão da estrutura + eixo de corte de cada nó ----------
    void imprimir(NoKD* no, int profundidade = 0) {
        if (no == nullptr) return;

        imprimir(no->direita, profundidade + 1);

        int eixo = profundidade % k;
        cout << string(profundidade * 4, ' ') << "(";
        for (size_t i = 0; i < no->ponto.coord.size(); i++) {
            cout << no->ponto.coord[i];
            if (i + 1 < no->ponto.coord.size()) cout << ", ";
        }
        cout << ")  [corta na dimensao " << eixo << "]" << endl;

        imprimir(no->esquerda, profundidade + 1);
    }

    void destruir(NoKD* no) {
        if (no == nullptr) return;
        destruir(no->esquerda);
        destruir(no->direita);
        delete no;
    }

    // ---------- Exportacao para Graphviz (.dot) ----------
    // contadorDot atribui um id sequencial a cada no, na mesma ordem em que
    // ele e' visitado. Cada no diz, ANTES de descer pro filho, qual id esse
    // filho vai receber (o proximo valor de contadorDot) -- e' assim que
    // conseguimos desenhar a aresta pai->filho sem precisar de uma segunda
    // passada pela arvore.
    int contadorDot = 0;

    void escreverDot(NoKD* no, ofstream &arquivo, int profundidade) {
        if (no == nullptr) return;

        int idAtual = contadorDot++;
        int eixo = profundidade % k;

        arquivo << "  n" << idAtual << " [label=\"(";
        for (size_t i = 0; i < no->ponto.coord.size(); i++) {
            arquivo << no->ponto.coord[i];
            if (i + 1 < no->ponto.coord.size()) arquivo << ", ";
        }
        arquivo << ")\\neixo " << eixo << "\"];\n";

        if (no->esquerda != nullptr) {
            int idFilho = contadorDot; // proximo id sera este, pois e' o proximo a ser atribuido
            arquivo << "  n" << idAtual << " -> n" << idFilho << " [label=\"<\"];\n";
            escreverDot(no->esquerda, arquivo, profundidade + 1);
        }
        if (no->direita != nullptr) {
            int idFilho = contadorDot;
            arquivo << "  n" << idAtual << " -> n" << idFilho << " [label=\">=\"];\n";
            escreverDot(no->direita, arquivo, profundidade + 1);
        }
    }

public:
    // k precisa ser conhecido na construcao: e' ele que define quantos
    // eixos existem para alternar em profundidade % k.
    KDTree(int dimensao) : raiz(nullptr), k(dimensao) {}
    ~KDTree() { destruir(raiz); }

    int dimensao() const { return k; }

    void inserir(const Ponto& p) { raiz = inserir(raiz, p, 0); }
    bool buscar(const Ponto& p) { return buscar(raiz, p, 0); }

    Ponto vizinhoMaisProximo(const Ponto& alvo) {
        NoKD* melhor = nullptr;
        double melhorDist = numeric_limits<double>::max();
        vizinhoMaisProximo(raiz, alvo, 0, melhor, melhorDist);
        return melhor ? melhor->ponto : Ponto();
    }

    void imprimir() {
        cout << "----- estrutura da KD-Tree (" << k << "-dimensional; raiz corta na dimensao 0) -----" << endl;
        imprimir(raiz);
        cout << "---------------------------------------------------" << endl;
    }

    // Exporta a arvore para um arquivo .dot (formato Graphviz). O rotulo do
    // grafico ja identifica que e' uma KD-Tree e a dimensao, pra nao ter
    // duvida nenhuma de qual estrutura e' quando alguem abrir o arquivo (ou
    // a imagem gerada a partir dele) depois. "tituloExtra" e' opcional, pra
    // diferenciar varias exportacoes (ex.: "exemplo pequeno", "N=10000").
    void exportarDot(const string &caminho, const string &tituloExtra = "") {
        filesystem::path caminhoPath(caminho);
        if (caminhoPath.has_parent_path()) {
            filesystem::create_directories(caminhoPath.parent_path());
        }

        ofstream arquivo(caminho);
        if (!arquivo.is_open()) {
            cerr << "Aviso: nao foi possivel criar \"" << caminho << "\" para exportar o .dot.\n";
            return;
        }

        contadorDot = 0;
        arquivo << "digraph KDTree {\n";
        arquivo << "  labelloc=\"t\";\n";
        arquivo << "  fontsize=16;\n";
        arquivo << "  label=\"KD-Tree (k=" << k << ")"
                << (tituloExtra.empty() ? "" : " -- " + tituloExtra) << "\";\n";
        arquivo << "  node [shape=box, style=filled, fillcolor=\"#eaf2ff\", fontname=\"Helvetica\", fontsize=11];\n";
        arquivo << "  edge [fontname=\"Helvetica\", fontsize=10];\n";

        if (raiz != nullptr) {
            escreverDot(raiz, arquivo, 0);
        }

        arquivo << "}\n";
        arquivo.close();

        cout << "Arvore exportada em formato Graphviz para \"" << caminho << "\".\n";
        cout << "  Para gerar a imagem: dot -Tpng " << caminho << " -o "
             << caminho.substr(0, caminho.find_last_of('.')) << ".png\n";
    }
};

int main(int argc, char* argv[]) {
    // "input/dados_kdtree.dat" e relativo ao diretorio de onde o executavel
    // e rodado -- execute a partir da pasta EmCpp (onde fica a pasta input/).
    // Opcionalmente aceita o caminho do arquivo como argumento, pra rodar
    // com os tamanhos gerados em input/gerados/ sem precisar recompilar:
    //   ./build/KDTree input/gerados/dados_kdtree_10000.dat
    //
    // A dimensao k NAO e mais fixa em 2: e' inferida automaticamente da
    // quantidade de numeros na primeira linha de dado do arquivo (paraPontosND).
    // Se o arquivo tiver "x y" por linha, k=2 (compativel com os dados atuais);
    // se tiver "x y z", k=3; "x y z w", k=4; etc. -- sem precisar mudar o codigo.
    string caminhoDados = (argc > 1) ? argv[1] : "input/dados_kdtree.dat";
    vector<Secao> secoes = lerSecoes(caminhoDados);

    vector<vector<double>> pontosParaInserir;
    vector<vector<double>> consultasBrutas;

    for (const Secao &secao : secoes) {
        if (secao.rotulo == "UNIFORME" || secao.rotulo == "CLUSTERS") {
            vector<vector<double>> pontos = paraPontosND(secao.dados);
            for (auto &p : pontos) pontosParaInserir.push_back(move(p));
        } else if (secao.rotulo == "QUERIES") {
            consultasBrutas = paraPontosND(secao.dados);
        }
    }

    if (pontosParaInserir.empty()) {
        cerr << "Nenhum ponto carregado -- verifique o arquivo de entrada." << endl;
        return 1;
    }

    // dimensao inferida do primeiro ponto lido
    int k = (int)pontosParaInserir[0].size();
    cout << "Dimensao inferida dos dados: k = " << k << "\n";

    KDTree arvore(k);

    cout << "Inserindo " << pontosParaInserir.size() << " pontos...\n";
    auto inicioInsercao = Relogio::now();
    for (auto &p : pontosParaInserir) {
        if ((int)p.size() != k) {
            cerr << "Aviso: ponto com dimensao diferente do esperado, ignorado.\n";
            continue;
        }
        arvore.inserir(Ponto(p));
    }
    auto fimInsercao = Relogio::now();
    double tempoInsercaoMs = chrono::duration<double, milli>(fimInsercao - inicioInsercao).count();
    cout << "  tempo de insercao: " << tempoInsercaoMs << " ms\n";
    gravarTempo(ARQUIVO_RESULTADOS, "KDTree", "k" + to_string(k), "insercao", (long long)pontosParaInserir.size(), tempoInsercaoMs);

    cout << "\nArvore completa montada (impressao em texto desativada; use exportarDot() se quiser inspecionar).\n";

    cout << "\nConsultas de vizinho mais proximo (bloco QUERIES):\n";
    auto inicioConsultas = Relogio::now();
    long long consultasValidas = 0;
    for (auto &q : consultasBrutas) {
        if ((int)q.size() != k) {
            cerr << "Aviso: consulta com dimensao diferente do esperado, ignorada.\n";
            continue;
        }
        consultasValidas++;
        Ponto alvo(q);
        Ponto vizinho = arvore.vizinhoMaisProximo(alvo);

        cout << "(";
        for (size_t i = 0; i < alvo.coord.size(); i++) {
            cout << alvo.coord[i];
            if (i + 1 < alvo.coord.size()) cout << ", ";
        }
        cout << ") -> vizinho: (";
        for (size_t i = 0; i < vizinho.coord.size(); i++) {
            cout << vizinho.coord[i];
            if (i + 1 < vizinho.coord.size()) cout << ", ";
        }
        cout << ")\n";
    }
    auto fimConsultas = Relogio::now();
    double tempoConsultasMs = chrono::duration<double, milli>(fimConsultas - inicioConsultas).count();
    cout << "  tempo total das consultas: " << tempoConsultasMs << " ms\n";
    gravarTempo(ARQUIVO_RESULTADOS, "KDTree", "k" + to_string(k), "vizinho_mais_proximo", consultasValidas, tempoConsultasMs);

    // ---------- Demonstracao pequena e exportavel (Secao 3) ----------
    // A arvore grande (milhares de pontos) e' otima pra medir tempo, mas
    // ilegivel como diagrama. Aqui, uma arvore pequena e fixa (so pra
    // ilustrar), exportada em .dot -- que vira uma imagem de verdade com
    // Graphviz, em vez de ficar presa ao terminal.
    cout << "\n\n===== Demonstracao pequena (exemplo p/ Secao 3) =====\n";
    KDTree exemplo(2);
    vector<vector<double>> pontosExemplo = {
        {30, 40}, {5, 25}, {70, 70}, {10, 12}, {50, 30},
        {35, 45}, {68, 20}, {22, 90}, {80, 10}, {55, 60}
    };
    for (auto &p : pontosExemplo) exemplo.inserir(Ponto(p));

    cout << "Pontos inseridos no exemplo: ";
    for (auto &p : pontosExemplo) cout << "(" << p[0] << "," << p[1] << ") ";
    cout << "\n";

    exemplo.exportarDot("resultados/diagramas/kdtree_exemplo.dot", "exemplo pequeno, 10 pontos");

    return 0;
}