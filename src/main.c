#include "raylib.h"
#include "raymath.h"

#define STB_DS_IMPLEMENTATION
#include "../third-party/stb/stb_ds.h"

#define NOB_IMPLEMENTATION
#include "../nob.h"

#if 0
#define SCREEN_WIDTH 2140 
#define SCREEN_HEIGHT 1440
#define SRC_CARD_SCALE 1
#else
#define SCREEN_WIDTH 1600 
#define SCREEN_HEIGHT 960
#define SRC_CARD_SCALE .5
#endif

#define SRC_CARD_WIDTH 200
#define SRC_CARD_HEIGHT 350
#define SRC_CARD_SPACING_X 8
#define SRC_CARD_SPACING_Y 5

#define CARD_WIDTH (SRC_CARD_WIDTH*SRC_CARD_SCALE+10)
#define CARD_HEIGHT (SRC_CARD_HEIGHT*SRC_CARD_SCALE)

#define FILES_COUNT 7
#define PILES_WIDTH CARD_WIDTH*1.25
#define PILES_HEIGHT CARD_HEIGHT*1.15
#define PILES_SPACING 20

typedef enum {
  SUIT_CLUBS,
  SUIT_DIAMONDS,
  SUIT_HEARTS,
  SUIT_SPADES,
  SUIT_COUNT
} Suit;

typedef enum {
  VAL_ACE = 1,
  VAL_TWO,
  VAL_THREE,
  VAL_FOUR,
  VAL_FIVE,
  VAL_SIX,
  VAL_SEVEN,
  VAL_EIGHT,
  VAL_NINE,
  VAL_TEN,
  VAL_JACK,
  VAL_QUEEN,
  VAL_KING,
  VAL_COUNT
} Value;

typedef enum {
  BC_RED,
  BC_BLUE,
  BC_YELLOW,
  BC_GREEN,
  BC_BLACK,
  BC_COUNT
} BackColor;

typedef enum {
  DECK_STD,
  DECK_DISCARD,
  DECK_FILE,
  DECK_COUNT
} DeckKind;

typedef struct {
  Suit suit;
  Value value;
  Rectangle source;
  Rectangle bounds;
  Vector2 origPos;
  bool drawn;
  bool flipped;
  bool moved;
} Card;

typedef struct {
  Card *items;
  size_t capacity;
  size_t count;
  DeckKind kind;
  Vector2 position;
  Vector2 cardStart;
  Rectangle bounds;
} Deck;

typedef enum {
  BK_MEANDER_FILL,
  BK_MEANDER_BORDER,
  BK_COUNT
} BackKind;

typedef struct {
  Rectangle source;
  Rectangle bounds;
} Back;

typedef struct {
  int key;
  Back value;
} Backs;

typedef struct {
  Deck *items;
  size_t capacity;
  size_t count;
} DeckFiles;

typedef struct {
  Deck deck;
  Deck drawn;
  Card *hoveredCard;
  Card *activeCard;
  Backs *backs;
  Back *activeBack;
  DeckFiles files;
  Deck *hoveredFile;
  Deck *homeFile;
} GameState;

bool CreateSTDDeck(Deck *deck) {
  if (deck->kind != DECK_STD) {
    nob_log(NOB_ERROR, "Invalid deck kind for CreateSTDDeck");
    return false;
  }
  for (int s = SUIT_CLUBS; s < SUIT_COUNT; ++s) {
    for (int v = VAL_ACE; v < VAL_COUNT; ++v) {
      Rectangle src = { 
        .x = (v-1) * (SRC_CARD_WIDTH + SRC_CARD_SPACING_X), 
        .y = s * (SRC_CARD_HEIGHT + SRC_CARD_SPACING_Y), 
        .width = SRC_CARD_WIDTH, 
        .height = SRC_CARD_HEIGHT 
      };
      Rectangle bounds = { .x = 0, .y = 0, .width = CARD_WIDTH, .height = CARD_HEIGHT };
      Card c = { 
        .suit = s, 
        .value = v,
        .source = src,
        .bounds = bounds,
        .drawn = false,
        .flipped = false,
      };
      nob_da_append(deck, c);
    }
  }
  return true;
}

void CreateBacks(Backs **backs, BackKind bk) {
  size_t y = bk * (SRC_CARD_HEIGHT + SRC_CARD_SPACING_Y);
  for (int b = BC_RED; b < BC_COUNT; ++b) {
    Back back = { 
      .source = CLITERAL(Rectangle) { 
        .x = b * (SRC_CARD_WIDTH + SRC_CARD_SPACING_X), 
        .y = y, 
        .width = SRC_CARD_WIDTH, 
        .height = SRC_CARD_HEIGHT 
      },
      .bounds = CLITERAL(Rectangle) { .x = 0, .y = 0, .width = CARD_WIDTH, .height = CARD_HEIGHT }
    };
    hmput(*backs, b, back);
  }
}

bool DrawDeckItemToScreen(Texture2D tex, Rectangle bounds, Rectangle src, Vector2 mouse) {
  DrawTexturePro(tex, src, bounds, Vector2Zero(), 0, WHITE);
  return CheckCollisionPointRec(mouse, bounds);
}

void DrawHoveredOutline(Rectangle bounds) {
  DrawRectangleLinesEx(bounds, 5, LIME);
}

Card *GetNextCard(Deck *src, Deck *dest) {
  nob_da_append(dest, src->items[0]);
  src->count--;
  if (src->count == 0) return &dest->items[dest->count-1];
  for (size_t c = 1; c < src->count-1; ++c) {
    src->items[c-1] = src->items[c];
  }
  return &dest->items[dest->count-1];
}

void AddCardToDeck(Card *card, Deck *deck) {
  nob_da_append(deck, *card); 
}

void RemoveCardFromDeck(Card *cardToRemove, Deck *deck) {
  Deck temp = {0};
  for (size_t c = 0; c < deck->count; ++c) {
    Card card = deck->items[c];
    if (card.suit != cardToRemove->suit && card.value != cardToRemove->value)
      nob_da_append(&temp, card);
  }
  deck->count = 0;
  for (size_t c= 0; c < temp.count; ++c) {
    nob_da_append(deck, temp.items[c]);
  }
}

Card *GetRandomCard(Deck *deck) {
  Card *card = NULL;
  while (!card) {
    int v = GetRandomValue(0, deck->count);
    if (!deck->items[v].drawn) {
      card = &deck->items[v];
      card->drawn = true;
      break;
    }
  }
  return card;
}

void ShuffleDeck(Deck *deck) {
  Deck temp = {0};
  for (size_t c = 0; c < deck->count; ++c) {
    Card *card = GetRandomCard(deck);
    nob_da_append(&temp, *card);
  }
  deck->count = 0;
  for (size_t c = 0; c < temp.count; ++c) {
    Card card = temp.items[c];
    nob_da_append(deck, card);
  }
}

const char* sizetToString(size_t num, size_t len) {
  char* buffer = (char*)malloc((len+1)*sizeof(char));
  if (!buffer) return NULL;
  snprintf(buffer, (len+1), "%ld", num);
  return buffer;
}

void UpdatePosition(Card *card, Vector2 delta) {
  if (!card->moved) {
    card->origPos.x = card->bounds.x;
    card->origPos.y = card->bounds.y;
    card->moved = true;
  }
  Vector2 pos = { .x = card->bounds.x, .y = card->bounds.y };
  pos = Vector2Add(pos, delta);
  card->bounds.x = pos.x;
  card->bounds.y = pos.y;
}

void ResetPosition(Card *card) {
  card->bounds.x = card->origPos.x;
  card->bounds.y = card->origPos.y;
  card->moved = false;
}

void SetPosition(Card *card, Vector2 pos) {
  card->bounds.x = pos.x;
  card->bounds.y = pos.y;
  card->origPos.x = pos.x;
  card->origPos.y = pos.y;
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cards");

  Image cardsImg = LoadImage("./assets/cards.png");
  Texture cardsTexture = LoadTextureFromImage(cardsImg);
  UnloadImage(cardsImg);
  
  Deck deck = {0};
  deck.kind = DECK_STD;
  if (!CreateSTDDeck(&deck)) return 1;
  ShuffleDeck(&deck);
  Deck drawn = {0};
  drawn.kind = DECK_DISCARD;
  drawn.bounds = CLITERAL(Rectangle) { .x = 10, .y = 20, .width = PILES_WIDTH, .height = PILES_HEIGHT }; 
  Image backsImg = LoadImage("./assets/backs.png");
  Texture backsTexture = LoadTextureFromImage(backsImg);
  UnloadImage(backsImg);
  Backs *backs = {0};
  CreateBacks(&backs, BK_MEANDER_BORDER);

  DeckFiles deckFiles = {0};

  GameState gs = {0};
  gs.deck = deck;
  gs.drawn = drawn;
  gs.backs = backs;
  gs.activeCard = NULL;
  gs.hoveredCard = NULL;
  gs.activeBack = &hmget(backs, BC_BLUE);
  gs.files = deckFiles;
  gs.hoveredFile = NULL;
  gs.homeFile = NULL;
  
  gs.activeBack->bounds.x = gs.drawn.bounds.x + (gs.drawn.bounds.width-(CARD_WIDTH))/2;
  gs.activeBack->bounds.y = gs.drawn.bounds.y + (gs.drawn.bounds.height-(CARD_HEIGHT))/2;

  size_t total_x = (FILES_COUNT*PILES_WIDTH)+((FILES_COUNT-1)*PILES_SPACING);

  size_t fx = (GetScreenWidth() - total_x) / 2;
  size_t fy = 20 + PILES_HEIGHT + 50;
  for (size_t f = 0; f < FILES_COUNT; ++f) {
    Deck d = {0};
    d.kind = DECK_FILE;
    d.position = CLITERAL(Vector2) { .x = fx + (PILES_WIDTH * f) + (PILES_SPACING * f), .y = fy };
    d.bounds = CLITERAL(Rectangle) { .x = d.position.x, .y = d.position.y, .width = PILES_WIDTH, .height = GetScreenHeight()-fy }; 
    for (size_t c = 0; c < f+1; ++c) {
      Card *card = GetNextCard(&gs.deck, &d);
      if (c == f)
        card->flipped = true;
      card->bounds.x = d.position.x + (PILES_WIDTH-CARD_WIDTH)/2; 
      card->bounds.y = (d.position.y + (PILES_HEIGHT-CARD_HEIGHT)/2) + (PILES_SPACING * 2) * c;
    }
    nob_da_append(&gs.files, d);
  }

  while(!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    gs.hoveredCard = NULL;
    gs.hoveredFile = NULL;

    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();

    if (gs.activeCard) {
      gs.hoveredCard = gs.activeCard;
      UpdatePosition(gs.activeCard, delta);
    }

    for (size_t f = 0; f < gs.files.count; ++f) {
      Deck d = gs.files.items[f];
      if (CheckCollisionPointRec(mouse, d.bounds)) {
        DrawRectangleRec(d.bounds, BLUE);
        gs.hoveredFile = &gs.files.items[f];
      }
    }

    DrawRectangleLinesEx(gs.drawn.bounds, 5, DARKPURPLE);
    
    if (gs.deck.count > 0) {
      DrawDeckItemToScreen(backsTexture, gs.activeBack->bounds, gs.activeBack->source, mouse);

      if (CheckCollisionPointRec(mouse, gs.drawn.bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Card *card = GetNextCard(&gs.deck, &gs.drawn); 
        card->bounds.x = gs.drawn.bounds.x + gs.drawn.bounds.width + 25;
        card->bounds.y = gs.activeBack->bounds.y;
        card->flipped = true;
      }
    } else {
      if (CheckCollisionPointRec(mouse, gs.drawn.bounds) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) & !gs.activeCard) {
        for (size_t c = 0; c < gs.drawn.count; ++c) {
          Card card = gs.drawn.items[c];
          card.drawn = false;
          card.flipped = false;
          nob_da_append(&gs.deck, card);
        }
        gs.drawn.count = 0;
      }
    }

    const char* text = sizetToString(gs.deck.count, 2);
    DrawText(text, gs.drawn.bounds.x, gs.drawn.bounds.y+gs.drawn.bounds.height+10, 30, LIME);

    for (size_t c = 0; c < gs.drawn.count; ++c) {
      Card card = gs.drawn.items[c];
      if (DrawDeckItemToScreen(cardsTexture, card.bounds, card.source, mouse) && !gs.activeCard) 
        gs.hoveredCard = &gs.drawn.items[c];
    }

    for (size_t f = 0; f < gs.files.count; ++f) {
      Deck d = gs.files.items[f];
      Rectangle r = { .x = d.position.x, .y = d.position.y, .width = PILES_WIDTH, .height = PILES_HEIGHT };
      DrawRectangleLinesEx(r, 5, DARKPURPLE);
      for (size_t c = 0; c < d.count; ++c) {
        Card card = d.items[c];
        if (card.flipped) {
          if (DrawDeckItemToScreen(cardsTexture, card.bounds, card.source, mouse) && !gs.activeCard)
            gs.hoveredCard = &d.items[c];
        } else {
          DrawDeckItemToScreen(backsTexture, card.bounds, gs.activeBack->source, mouse);
        }
      }
    }

    if (gs.hoveredCard) {
      DrawHoveredOutline(gs.hoveredCard->bounds);
      if (!gs.activeCard) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
          gs.activeCard = gs.hoveredCard;
          if (gs.hoveredFile)
            gs.homeFile = gs.hoveredFile;
          if (CheckCollisionPointRec(mouse, gs.hoveredCard->bounds))
            gs.homeFile = &gs.drawn;
        }
      } else {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
          if (gs.hoveredFile) {
            if (gs.hoveredFile->count == 0) {
              SetPosition(gs.activeCard, gs.hoveredFile->cardStart);
            } else {
              Card last = gs.hoveredFile->items[gs.hoveredFile->count-1];
              Vector2 pos = { .x = last.bounds.x, .y = last.bounds.y };
              pos.y += (PILES_SPACING*2);
              RemoveCardFromDeck(gs.activeCard, gs.homeFile);
              nob_log(NOB_INFO, "%ld", gs.homeFile->count);
              //nob_log(NOB_INFO, "%d, %d", gs.homeFile->items[gs.homeFile->count-1].suit, gs.homeFile->items[gs.homeFile->count-1].value);
              if (gs.homeFile->count > 0 && !gs.homeFile->items[gs.homeFile->count-1].flipped) {
                gs.homeFile->items[gs.homeFile->count-1].flipped = true;
              }
              SetPosition(gs.activeCard, pos);
              AddCardToDeck(gs.activeCard, gs.hoveredFile);
            }
          } else {
            ResetPosition(gs.activeCard);
          }
          gs.activeCard = NULL;
          gs.homeFile = NULL;
        } else {
          DrawDeckItemToScreen(cardsTexture, gs.activeCard->bounds, gs.activeCard->source, mouse);
        }
      }
    }

    EndDrawing();
  }

  UnloadTexture(cardsTexture);
  UnloadTexture(backsTexture);

  CloseWindow();
}
