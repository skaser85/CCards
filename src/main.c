#include "raylib.h"
#include "raymath.h"

#define NOB_IMPLEMENTATION
#include "../nob.h"

#define CARD_WIDTH 200
#define CARD_HEIGHT 350
#define CARD_SPACING 8
#define CARD_SCALE 0.35

#define HEARTS_Y 0
#define DIAMONDS_Y 355
#define SPADES_Y 710
#define CLUBS_Y 1065

typedef enum {
  SUIT_CLUB,
  SUIT_DIAMOND,
  SUIT_HEART,
  SUIT_SPADE,
  SUIT_COUNT
} Suit;

typedef enum {
  VAL_ACE,
  VAL_KING,
  VAL_QUEEN,
  VAL_JACK,
  VAL_TEN,
  VAL_NINE,
  VAL_EIGHT,
  VAL_SEVEN,
  VAL_SIX,
  VAL_FIVE,
  VAL_FOUR,
  VAL_THREE,
  VAL_TWO,
  VAL_COUNT
} Value;

typedef struct {
  Suit suit;
  Value value;
  Rectangle source;
  Vector2 position;
} Card;

typedef struct {
  Card *items;
  size_t capacity;
  size_t count;
} Deck;

void CreateDeck(Deck *deck) {
  for (int s = SUIT_CLUB; s < SUIT_COUNT; ++s) {
    size_t x = 0;
    size_t y = 0;
    switch (s) {
      case SUIT_CLUB: y = CLUBS_Y; break;
      case SUIT_DIAMOND: y = DIAMONDS_Y; break;
      case SUIT_HEART: y = HEARTS_Y; break;
      case SUIT_SPADE: y = SPADES_Y; break;
    }

    for (int v = VAL_ACE; v < VAL_COUNT; ++v) {
      Rectangle source = { .x = x, .y = y, .width = CARD_WIDTH, .height = CARD_HEIGHT };
      Card c = { .suit = s, .value = v, .source = source, .position = Vector2Zero() };
      nob_da_append(deck, c);
      x += (CARD_WIDTH + CARD_SPACING);
    }
  }

  float x = 20;
  float y = 10;
  for (size_t c = 0; c < deck->count; ++c) {
    if (c > 0 && c % 13 == 0) {
      y += (CARD_HEIGHT*CARD_SCALE) + 10;
      x = 20;
    }
    deck->items[c].position = CLITERAL(Vector2) { x, y };
    x += (CARD_WIDTH*CARD_SCALE)+10;
  }
}

int main(void) {
  InitWindow(1200, 720, "Cards");

  Image cardsImg = LoadImage("./assets/cards.png");
  Texture cardsTexture = LoadTextureFromImage(cardsImg);
  UnloadImage(cardsImg);

  Deck deck = {0};
  CreateDeck(&deck);

  Card *activeCard = NULL;

  while(!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();
    
    if (activeCard != NULL) {
      Rectangle bounds = { .x = activeCard->position.x, .y = activeCard->position.y, .width = CARD_WIDTH*CARD_SCALE, .height = CARD_HEIGHT*CARD_SCALE };
      if (!CheckCollisionPointRec(mouse, bounds)) {
        activeCard = NULL;
      } else {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          activeCard->position = Vector2Add(activeCard->position, delta);
        }
      } 
    }

    for (size_t c = 0; c < deck.count; ++c) {

      Card *card = &deck.items[c];
      Rectangle bounds = { .x = card->position.x, .y = card->position.y, .width = CARD_WIDTH*CARD_SCALE, .height = CARD_HEIGHT*CARD_SCALE };
      DrawTexturePro(cardsTexture, card->source, bounds, Vector2Zero(), 0, WHITE);
      
      if (activeCard != NULL && activeCard == card) {
        DrawRectangleLinesEx(bounds, 3, LIME);
      } else {
        if (CheckCollisionPointRec(mouse, bounds)) {
          activeCard = card;
        }
      }
      
    }

    EndDrawing();
  }

  UnloadTexture(cardsTexture);

  CloseWindow();
}
