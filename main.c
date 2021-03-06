
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_mixer.h"

#define CHECK_NEG(op)      do { if ((op) < 0)  raler(strerror(errno)); } while (0)
#define CHECK_NULL(op)     do { if ((op) == NULL)  raler(strerror(errno)); } while (0)
#define CHECK_NULL_SDL(op) do { if ((op) == NULL) afficheErr(SDL_GetError()); } while(0)
#define CHECK_NEG_SDL(op)  do { if ((op) < 0) afficheErr(SDL_GetError()); } while(0)
#define CHECK_NULL_MIX(op) do { if ((op) == NULL) afficheErr(Mix_GetError()); } while(0) //Mixer
#define CHECK_NEG_MIX(op)  do { if ((op) < 0) afficheErr(Mix_GetError()); } while(0) //Mixer
#define CHECK_NULL_TTF(op) do { if ((op) == NULL) afficheErr(TTF_GetError()); } while(0)
#define CHECK_NEG_TTF(op)  do { if ((op) < 0) afficheErr(TTF_GetError()); } while(0)
#define CHECK_NULL_IMG(op) do { if ((op) == NULL) afficheErr(IMG_GetError()); } while(0)
#define CHECK_NEG_IMG(op)  do { if ((op) < 0) afficheErr(IMG_GetError()); } while(0)

#define LONGUEUR_MAX 128
#define LONGUEUR_BASE 4

#define LONGUEUR_FENETRE 1080
#define HAUTEUR_FENETRE 720

enum direction
{
    HAUT,
    BAS,
    GAUCHE,
    DROITE
};

enum etat
{
    TITRE,
    JEU,
    RETRY
};

void raler(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void afficheErr(const char* erreur_sdl)
{
    CHECK_NEG(fprintf(stderr, "%s\n", erreur_sdl));
    exit(EXIT_FAILURE);
}

SDL_Window* fenetre = NULL;
SDL_Renderer* rendu = NULL;

TTF_Font* police = NULL;

SDL_Rect serpent[LONGUEUR_MAX] = {{0,0,0,0}};
SDL_Rect pomme = {0,0,0,0};
SDL_Rect dest_score = {0,0,0,0};
SDL_Rect dest_valeur = {0,0,0,0};
SDL_Rect dest_retry = {0,0,0,0};
SDL_Rect dest_appuyer = {0,0,0,0};
SDL_Rect src_appuyer = {0,0,0,0};

enum direction direction = DROITE;
bool jouer = true;
char valeur_score[4];
int longueur = LONGUEUR_BASE;
int previous_x = 0, previous_y = 0, score = 0;
unsigned int lastTime = 0, currentTime = 0;
int etat = TITRE;

SDL_Texture* text_score = NULL;
SDL_Texture* text_valeur = NULL;
SDL_Texture* text_retry = NULL;
SDL_Texture* text_appuyer = NULL;

SDL_Color blanc = {255, 255, 255, 0};

Mix_Chunk* son = NULL;
Mix_Chunk* entree = NULL;

SDL_Texture* creerTexture(const char* image, SDL_Renderer* rendu)
{
    SDL_Surface* surface = NULL;
    CHECK_NULL_IMG((surface = IMG_Load(image)));
    SDL_Texture* texture = NULL;
    CHECK_NULL_SDL((texture = SDL_CreateTextureFromSurface(rendu, surface)));
    SDL_FreeSurface(surface);
    return texture;
}

SDL_Rect init_rect(int x, int y, int w, int h)
{
    SDL_Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

void random_apple()
{
    pomme = init_rect(rand() % (LONGUEUR_FENETRE / 40) * 40, rand() % (HAUTEUR_FENETRE/ 40) * 40, 40, 40);
}

void reset()
{
    longueur = LONGUEUR_BASE;
    random_apple();
    for(int i = 0; i < LONGUEUR_BASE; i++)
        serpent[i] = init_rect(200 - (i * 40), 200, 40, 40);
    direction = DROITE;
    score = 0;
}

void init()
{
    srand(time(NULL));

    CHECK_NEG_SDL(SDL_Init(SDL_INIT_EVERYTHING));
    CHECK_NEG_IMG(IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG));
    CHECK_NEG_TTF(TTF_Init());
    CHECK_NEG_MIX(Mix_Init(MIX_INIT_MP3));

    CHECK_NULL_SDL(fenetre = SDL_CreateWindow("Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LONGUEUR_FENETRE, HAUTEUR_FENETRE, SDL_WINDOW_ALWAYS_ON_TOP));
    CHECK_NULL_SDL(rendu = SDL_CreateRenderer(fenetre, -1, SDL_RENDERER_PRESENTVSYNC));

    random_apple();

    for(int i = 0; i < LONGUEUR_BASE; i++)
        serpent[i] = init_rect(200 - (i * 40), 200, 40, 40);

    dest_score = init_rect(900, 20, 100, 50);
    dest_valeur = init_rect(1000, 30, 30, 40);
    dest_retry = init_rect(300, 300, 500, 200);
    dest_appuyer = init_rect(30, 50, 480, 80);

    CHECK_NULL_TTF(police = TTF_OpenFont("police.ttf", 40));

    CHECK_NULL_SDL(text_score = SDL_CreateTextureFromSurface(rendu, TTF_RenderText_Blended(police, "Score : ", blanc)));
    CHECK_NULL_SDL(text_retry = SDL_CreateTextureFromSurface(rendu, TTF_RenderText_Blended(police, "Recommencer ?", blanc)));
    text_appuyer = creerTexture("appuyer_entrer.png", rendu);

    CHECK_NEG_MIX(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024));
    CHECK_NULL_MIX(son = Mix_LoadWAV("son_menu.mp3"));
    CHECK_NULL_MIX(entree = Mix_LoadWAV("entree.wav"));
}

void verifieActionUtilisateur()
{
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0)
    {
        switch(e.type)
        {
            case SDL_WINDOWEVENT :

                if(e.window.event == SDL_WINDOWEVENT_CLOSE)
                    jouer = false;
                break;

            case SDL_KEYDOWN :

                if(e.key.keysym.sym == SDLK_ESCAPE)
                    jouer = false;
                if(etat == TITRE)
                {
                    if(e.key.keysym.sym == SDLK_RETURN)
                    {
                        etat = JEU;
                        CHECK_NEG_MIX(Mix_PlayChannel(1, entree, 0));
                    }
                }
                else if(etat == JEU)
                {
                    if(e.key.keysym.sym == SDLK_UP && direction != BAS)
                        direction = HAUT;
                    else if(e.key.keysym.sym == SDLK_DOWN && direction != HAUT)
                        direction = BAS;
                    else if(e.key.keysym.sym == SDLK_LEFT && direction != DROITE)
                        direction = GAUCHE;
                    else if(e.key.keysym.sym == SDLK_RIGHT && direction != GAUCHE)
                        direction = DROITE;
                }
                else if(etat == RETRY)
                {
                    if(e.key.keysym.sym == SDLK_RETURN)
                    {
                        etat = JEU;
                        CHECK_NEG_MIX(Mix_PlayChannel(1, entree, 0));
                    }
                }
                break;

            case SDL_MOUSEBUTTONDOWN :

                if(e.button.button == SDL_BUTTON_LEFT)
                {
                    if(etat == TITRE || etat == RETRY)
                    {
                        etat = JEU;
                        CHECK_NEG_MIX(Mix_PlayChannel(1, entree, 0));
                    }
                }
                break;

            default:
                break;
        }
    }
}

void update()
{
    if(etat == JEU)
    {
        CHECK_NEG(snprintf(valeur_score, 4, "%d", score));
        CHECK_NULL_SDL(text_valeur = SDL_CreateTextureFromSurface(rendu, TTF_RenderText_Blended(police, valeur_score, blanc)));

        //pomme
        if(serpent[0].x < pomme.x + pomme.w
        && serpent[0].x + serpent[0].w > pomme.x
        && serpent[0].y < pomme.y + pomme.h
        && serpent[0].h + serpent[0].y > pomme.y)
        {
            longueur += 1;
            score += 1;
            random_apple();
            serpent[longueur - 1] = init_rect(previous_x, previous_y, 40, 40);
            CHECK_NEG_MIX(Mix_PlayChannel(1, son, 0));
        }
        else
        {
            previous_x = serpent[longueur - 1].x;
            previous_y = serpent[longueur - 1].y;
        }

        //serpent sur lui meme
        for(int i = 1; i < longueur; i++)
        {
            if(serpent[0].x < serpent[i].x + serpent[i].w
            && serpent[0].x + serpent[0].w > serpent[i].x
            && serpent[0].y < serpent[i].y + serpent[i].h
            && serpent[0].h + serpent[0].y > serpent[i].y)
            {
                etat = RETRY;
                reset();
            }

        }

        //fenetre
        if(serpent[0].x >= LONGUEUR_FENETRE || serpent[0].x == 0 || serpent[0].y >= HAUTEUR_FENETRE || serpent[0].y == 0)
        {
            etat = RETRY;
            reset();
        }

        //deplacement
        currentTime = SDL_GetTicks();
        if (currentTime > lastTime + 180)
        {
            for(int i = longueur - 1; i > 0; i--)
                serpent[i] = serpent[i-1];
            if(direction == HAUT)
                serpent[0].y -= 40;
            else if(direction == BAS)
                serpent[0].y += 40;
            else if(direction == GAUCHE)
                serpent[0].x -= 40;
            else if(direction == DROITE)
                serpent[0].x += 40;
            lastTime = currentTime;
        }
    }
}

void draw()
{
    CHECK_NEG_SDL(SDL_SetRenderDrawColor(rendu, 0, 0, 0, 0));
    CHECK_NEG_SDL(SDL_RenderClear(rendu));
    if(etat == TITRE)
    {
        Uint32 sprite = (SDL_GetTicks() / 150) % 7;
        src_appuyer = init_rect(0, sprite * 80, 480, 80);
        CHECK_NEG_SDL(SDL_RenderCopy(rendu, text_appuyer, &src_appuyer, &dest_appuyer));
    }
    else if(etat == JEU)
    {
        CHECK_NEG_SDL(SDL_SetRenderDrawColor(rendu, 0, 255, 0, 0));
        for(int i = 0; i < longueur; i++)
            CHECK_NEG_SDL(SDL_RenderDrawRect(rendu, &serpent[i]));
        CHECK_NEG_SDL(SDL_SetRenderDrawColor(rendu, 255, 0, 0, 0));
        CHECK_NEG_SDL(SDL_RenderFillRect(rendu, &pomme));
        CHECK_NEG_SDL(SDL_RenderCopy(rendu, text_score, NULL, &dest_score));
        CHECK_NEG_SDL(SDL_RenderCopy(rendu, text_valeur, NULL, &dest_valeur));
    }
    else if(etat == RETRY)
        CHECK_NEG_SDL(SDL_RenderCopy(rendu, text_retry, NULL, &dest_retry));

    SDL_RenderPresent(rendu);
}

void liberation()
{
    TTF_CloseFont(police);
    Mix_CloseAudio();
    SDL_DestroyTexture(text_appuyer);
    SDL_DestroyTexture(text_retry);
    SDL_DestroyTexture(text_score);
    SDL_DestroyTexture(text_valeur);
    Mix_FreeChunk(son);
    Mix_FreeChunk(entree);
    SDL_DestroyRenderer(rendu);
    SDL_DestroyWindow(fenetre);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    init();

    while(jouer == true)
    {
        verifieActionUtilisateur();
        update();
        draw();
    }
    liberation();
    return EXIT_SUCCESS;
}
