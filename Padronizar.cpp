#include "Arvores.h"
#include <cctype>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

using namespace std;

// Tabela estatica: para cada segundo byte possivel de um acento (0x80 a 0xBF,
// entao indexamos por c2 - 0x80), guarda a letra "base" correspondente, ou
// '\0' se aquele byte nao for um acento conhecido. Construida uma unica vez
// (static), consultada em O(1) por acesso direto ao indice.
static const char TABELA_ACENTOS[64] = {
//  0x80 0x81 0x82 0x83 0x84 0x85 0x86 0x87 0x88 0x89 0x8A 0x8B 0x8C 0x8D 0x8E 0x8F
    'a', 'a', 'a', 'a', 'a', '\0','\0','c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
//  0x90 0x91 0x92 0x93 0x94 0x95 0x96 0x97 0x98 0x99 0x9A 0x9B 0x9C 0x9D 0x9E 0x9F
    '\0','n', 'o', 'o', 'o', 'o', 'o', '\0','\0','u', 'u', 'u', 'u', '\0','\0','\0',
//  0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 0xA6 0xA7 0xA8 0xA9 0xAA 0xAB 0xAC 0xAD 0xAE 0xAF
    'a', 'a', 'a', 'a', 'a', '\0','\0','c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
//  0xB0 0xB1 0xB2 0xB3 0xB4 0xB5 0xB6 0xB7 0xB8 0xB9 0xBA 0xBB 0xBC 0xBD 0xBE 0xBF
    '\0','n', 'o', 'o', 'o', 'o', 'o', '\0','\0','u', 'u', 'u', 'u', '\0','\0','\0'
};

string normalizar(const string &entrada) {
    string saida;
    saida.reserve(entrada.size());

    for (size_t i = 0; i < entrada.size(); i++) {
        unsigned char c = entrada[i];

        if (c >= 'A' && c <= 'Z') {
            saida += char(tolower(c));
        } else if (c >= 'a' && c <= 'z') {
            saida += char(c);
        } else if (c == 0xC3 && i + 1 < entrada.size()) {
            // inicio de caractere acentuado em UTF-8 (bloco Latin-1 Supplement)
            unsigned char c2 = entrada[++i];
            char base = TABELA_ACENTOS[c2 - 0x80];
            if (base != '\0') saida += base;
        }
        // qualquer outro byte (digito, espaco, pontuacao, emoji, etc.): ignorado, nao entra em 'saida'
    }

    return saida;
}

// ---------------------------------------------------------------------
// Leitura padronizada dos arquivos de entrada
// ---------------------------------------------------------------------

// Remove '\r' (e '\n' remanescente) do final da linha -- protege contra
// arquivos salvos com quebra de linha estilo Windows (CRLF).
static string limparLinha(string linha) {
    while (!linha.empty() && (linha.back() == '\r' || linha.back() == '\n')) {
        linha.pop_back();
    }
    return linha;
}

vector<Secao> lerSecoes(const string &caminho) {
    vector<Secao> secoes;
    ifstream arquivo(caminho);

    if (!arquivo.is_open()) {
        cerr << "Erro: nao foi possivel abrir o arquivo \"" << caminho << "\"." << endl;
        return secoes;
    }

    string linha;
    while (getline(arquivo, linha)) {
        linha = limparLinha(linha);

        if (linha.empty() || linha[0] == '#') {
            continue; // linha em branco ou comentario: ignora
        }

        // linha de cabecalho do bloco: "ROTULO quantidade"
        istringstream cabecalho(linha);
        string rotulo;
        size_t quantidade;
        if (!(cabecalho >> rotulo >> quantidade)) {
            continue; // linha mal formada, nao e um cabecalho valido: ignora
        }

        Secao secao;
        secao.rotulo = rotulo;
        secao.dados.reserve(quantidade);

        for (size_t i = 0; i < quantidade && getline(arquivo, linha); i++) {
            secao.dados.push_back(limparLinha(linha));
        }

        secoes.push_back(move(secao));
    }

    return secoes;
}

vector<int> paraInteiros(const vector<string> &linhas) {
    vector<int> valores;
    valores.reserve(linhas.size());

    for (const string &linha : linhas) {
        if (linha.empty()) continue;
        valores.push_back(stoi(linha));
    }

    return valores;
}

vector<pair<double, double>> paraPontos(const vector<string> &linhas) {
    vector<pair<double, double>> pontos;
    pontos.reserve(linhas.size());

    for (const string &linha : linhas) {
        if (linha.empty()) continue;
        istringstream ss(linha);
        double x, y;
        ss >> x >> y;
        pontos.emplace_back(x, y);
    }

    return pontos;
}

vector<vector<double>> paraPontosND(const vector<string> &linhas) {
    vector<vector<double>> pontos;
    pontos.reserve(linhas.size());

    for (const string &linha : linhas) {
        if (linha.empty()) continue;

        istringstream ss(linha);
        vector<double> ponto;
        double valor;
        while (ss >> valor) {
            ponto.push_back(valor);
        }

        if (!ponto.empty()) {
            pontos.push_back(move(ponto));
        }
    }

    return pontos;
}

vector<string> lerPalavras(const string &caminho) {
    vector<string> palavras;
    ifstream arquivo(caminho);

    if (!arquivo.is_open()) {
        cerr << "Erro: nao foi possivel abrir o arquivo \"" << caminho << "\"." << endl;
        return palavras;
    }

    string linha;
    while (getline(arquivo, linha)) {
        linha = limparLinha(linha);
        if (linha.empty() || linha[0] == '#') continue;
        palavras.push_back(normalizar(linha));
    }

    return palavras;
}

void gravarTempo(const string &arquivoCSV, const string &estrutura,
                  const string &bloco, const string &operacao,
                  long long quantidade, double tempoMs) {
    // garante que a pasta do CSV exista (ex.: "resultados/") -- senao o
    // ofstream abaixo falha silenciosamente
    filesystem::path caminho(arquivoCSV);
    if (caminho.has_parent_path()) {
        filesystem::create_directories(caminho.parent_path());
    }

    // ve se o arquivo ja existe (e ja tem cabecalho) antes de abrir em
    // modo append, senao cada chamada duplicaria o cabecalho
    ifstream teste(arquivoCSV);
    bool existe = teste.good();
    teste.close();

    ofstream arquivo(arquivoCSV, ios::app);
    if (!arquivo.is_open()) {
        cerr << "Aviso: nao foi possivel abrir \"" << arquivoCSV << "\" para gravar tempos." << endl;
        return;
    }

    if (!existe) {
        arquivo << "estrutura,bloco,operacao,quantidade,tempo_ms\n";
    }

    arquivo << estrutura << "," << bloco << "," << operacao << ","
            << quantidade << "," << tempoMs << "\n";
}