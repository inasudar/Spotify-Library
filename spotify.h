#define _CRT_SECURE_NO_WARNINGS

#ifndef SPOTIFY_H
#define SPOTIFY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LENGTH 100
#define INITIAL_CAPACITY 10

typedef struct {
    char title[MAX_LENGTH];
    char artist[MAX_LENGTH];
    char album[MAX_LENGTH];
    int duration;
    int year;
    int id;
} Song;

typedef struct {
    char name[MAX_LENGTH];
    Song** songs;
    int num_songs;
    int capacity;
} Artist;

typedef struct {
    char name[MAX_LENGTH];
    Song** songs;
    int num_songs;
    int capacity;
} Playlist;

typedef enum {
    MENU_EXIT = 0,
    MENU_ADD = 1,
    MENU_LIST = 2,
    MENU_SEARCH = 3,
    MENU_DELETE = 4,
    MENU_EDIT = 5,
    MENU_CREATE_PL = 6,
    MENU_ADD_TO_PL = 7,
    MENU_VIEW_PL = 8,
    MENU_SORT = 9,
    MENU_INSERT = 10
} Menu;

typedef enum {
    SORT_TITLE = 1,
    SORT_ARTIST = 2,
    SORT_YEAR = 3,
    SORT_DURATION = 4
} SortField;

typedef union {
    int seconds;
    struct { unsigned short m, s; } mmss;
} Duration;

Song* find_song_by_id_bsearch(int id);
Song* find_song_by_id_recursive(int id);

void format_duration_mmss(int seconds, char out[8]);

// Osnovne funkcije
void init_library(void);
void cleanup_library(void);

// Funkcije za pjesme
void add_song(void);
void delete_song(int id);
void search_songs(void);
void list_all_songs(void);
void edit_song(int id);
void reorganize_song_ids(void);

// Funkcije za izvođače
void list_artists(void);
void list_artist_songs(const char* artist_name);

// Funkcije za playliste
void create_playlist(void);
void add_to_playlist(void);
void list_playlists(void);
void show_playlist_songs(const char* playlist_name);

void append_song_to_file(Song* s);
void load_songs_from_file(void);
void print_library_stats(void);
void sort_songs_menu(void);

// Pomocne funkcije
void to_lower_case(char* str);
bool string_contains(const char* str, const char* substr);

extern Song** songs;
extern int num_songs;
extern int songs_capacity;

extern Playlist* playlists;
extern int num_playlists;
extern int playlists_capacity;

void sort_songs_by_title_asc(void);
long get_songs_file_size_bytes(void);
void print_songs_file_info(void);

// Kopiranje datoteka (za backup) i atomsko spremanje
int copy_file(const char* src, const char* dst);
int save_all_songs_to_file(void);

void sort_songs_by_artist_asc(void);
void add_song_insert_sorted_by_title(void);

static inline int generate_song_id(void) { return num_songs + 1; }

#endif // SPOTIFY_H
