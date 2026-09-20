#!/usr/bin/env python3
"""
Gerador de dados de entrada para Splay, Treap e KDTree, em varios tamanhos.

Reproduz o MESMO formato e as MESMAS ideias dos arquivos originais
(input/dados_splay.dat, dados_treap.dat, dados_kdtree.dat), so que
parametrizado por N -- pra poder gerar 100, 1000, 10000, 100000 (ou
qualquer outro tamanho) sem reescrever tudo na mao.

Uso:
    python3 gerar_dados.py
    (gera todos os tamanhos definidos em TAMANHOS, para as 3 estruturas,
     dentro de input/gerados/)

Formato de cada arquivo (igual ao original):
    # comentario (ignorado pelo lerSecoes)
    ROTULO quantidade
    dado_1
    dado_2
    ...
    dado_quantidade
"""

import random
import os

TAMANHOS = [100, 1000, 10000, 100000]
PASTA_SAIDA = os.path.join(os.path.dirname(__file__), "..", "input", "gerados")
SEED = 42  # fixa, para os arquivos serem reprodutiveis


def escrever_secao(f, rotulo, comentario, itens):
    f.write(f"# {comentario}\n")
    f.write(f"{rotulo} {len(itens)}\n")
    for item in itens:
        f.write(f"{item}\n")


# ---------------------------------------------------------------------
# Splay: INSERT (permutacao de 1..N), ACCESS (5N acessos, com uma fatia
# de chaves "quentes" acessada com muito mais frequencia -- e isso que
# evidencia o efeito do self-adjusting), HOTSET (as chaves quentes).
# ---------------------------------------------------------------------
def gerar_splay(n, rng):
    chaves = list(range(1, n + 1))
    rng.shuffle(chaves)  # ordem de insercao embaralhada

    tamanho_hotset = max(1, round(0.05 * n))  # 5% das chaves, igual ao original
    hotset = rng.sample(chaves, tamanho_hotset)
    hotset_set = set(hotset)

    # ACCESS: 5N acessos no total. 70% deles caem no HOTSET (repetidos,
    # criando localidade de referencia), 30% sao uniformes em toda a arvore.
    total_access = 5 * n
    n_quentes = round(0.7 * total_access)
    n_frios = total_access - n_quentes

    access = [rng.choice(hotset) for _ in range(n_quentes)] if hotset else []
    access += [rng.choice(chaves) for _ in range(n_frios)]
    rng.shuffle(access)

    return chaves, access, hotset


def escrever_arquivo_splay(caminho, n, rng):
    chaves, access, hotset = gerar_splay(n, rng)
    with open(caminho, "w") as f:
        escrever_secao(f, "INSERT", "chaves a inserir na arvore, uma por linha, na ordem dada", chaves)
        escrever_secao(f, "ACCESS", "sequencia de acessos (busca) para medir efeito do splaying", access)
        escrever_secao(f, "HOTSET", "chaves consideradas 'quentes' (para referencia/plot)", hotset)


# ---------------------------------------------------------------------
# Treap: RANDOM (permutacao aleatoria de 1..N), SORTED (1..N em ordem),
# REVERSE (N..1) -- os tres cenarios de ordem de insercao que o arquivo
# original ja usava.
# ---------------------------------------------------------------------
def escrever_arquivo_treap(caminho, n, rng):
    aleatorio = list(range(1, n + 1))
    rng.shuffle(aleatorio)
    crescente = list(range(1, n + 1))
    decrescente = list(range(n, 0, -1))

    with open(caminho, "w") as f:
        escrever_secao(f, "RANDOM", "insercao em ordem aleatoria", aleatorio)
        escrever_secao(f, "SORTED", "insercao em ordem crescente (pior caso p/ BST comum)", crescente)
        escrever_secao(f, "REVERSE", "insercao em ordem decrescente", decrescente)


# ---------------------------------------------------------------------
# KDTree: UNIFORME (N pontos uniformes em [0,100] x [0,100]), CLUSTERS
# (N pontos concentrados em alguns agrupamentos), QUERIES (pontos fixos
# de consulta -- nao precisa escalar com N).
# ---------------------------------------------------------------------
def gerar_pontos_uniformes(n, rng):
    return [f"{rng.uniform(0, 100):.2f} {rng.uniform(0, 100):.2f}" for _ in range(n)]


def gerar_pontos_clusters(n, rng, n_centros=5):
    centros = [(rng.uniform(15, 85), rng.uniform(15, 85)) for _ in range(n_centros)]
    pontos = []
    for _ in range(n):
        cx, cy = rng.choice(centros)
        x = min(100, max(0, rng.gauss(cx, 6)))
        y = min(100, max(0, rng.gauss(cy, 6)))
        pontos.append(f"{x:.2f} {y:.2f}")
    return pontos


QUERIES_PADRAO = [
    "10 10", "90 90", "50 50", "30 70", "70 30", "0 0", "100 100", "50 0",
]


def escrever_arquivo_kdtree(caminho, n, rng):
    uniforme = gerar_pontos_uniformes(n, rng)
    clusters = gerar_pontos_clusters(n, rng)

    with open(caminho, "w") as f:
        escrever_secao(f, "UNIFORME", "pontos uniformemente distribuidos em [0,100]x[0,100]", uniforme)
        escrever_secao(f, "CLUSTERS", "pontos concentrados em agrupamentos (pior caso p/ particionamento)", clusters)
        escrever_secao(f, "QUERIES", "consultas de vizinho mais proximo (fixas, nao escalam com N)", QUERIES_PADRAO)


def main():
    os.makedirs(PASTA_SAIDA, exist_ok=True)

    for n in TAMANHOS:
        # uma seed derivada de N, mas ainda reprodutivel a partir da SEED fixa
        rng = random.Random(f"{SEED}-{n}")

        caminho_splay = os.path.join(PASTA_SAIDA, f"dados_splay_{n}.dat")
        escrever_arquivo_splay(caminho_splay, n, rng)
        print(f"gerado: {caminho_splay}")

        caminho_treap = os.path.join(PASTA_SAIDA, f"dados_treap_{n}.dat")
        escrever_arquivo_treap(caminho_treap, n, rng)
        print(f"gerado: {caminho_treap}")

        caminho_kdtree = os.path.join(PASTA_SAIDA, f"dados_kdtree_{n}.dat")
        escrever_arquivo_kdtree(caminho_kdtree, n, rng)
        print(f"gerado: {caminho_kdtree}")


if __name__ == "__main__":
    main()
