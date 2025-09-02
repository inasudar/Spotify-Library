#include "spotify.h"

Song** songs = NULL;
int num_songs = 0;
int songs_capacity = 0;

Playlist* playlists = NULL;
int num_playlists = 0;
int playlists_capacity = 0;

static void sort_songs_by_duration_asc_simple(void);
static void sort_songs_by_year_with_range(int ascending);
static int cmp_song_id_key(const void* keyp, const void* elem);


static void log_errno(const char* ctx) {
    fprintf(stderr, "%s: (%d) %s\n", ctx, errno, strerror(errno));
}

void init_library(void) {
    songs_capacity = INITIAL_CAPACITY;
    songs = (Song**)xmalloc((size_t)songs_capacity * sizeof(*songs));

    playlists_capacity = INITIAL_CAPACITY;
    playlists = (Playlist*)xmalloc((size_t)playlists_capacity * sizeof(*playlists));

    for (int i = 0; i < playlists_capacity; ++i) {
        playlists[i].songs = NULL;
        playlists[i].num_songs = 0;
        playlists[i].capacity = 0;
        playlists[i].name[0] = '\0';
    }
}

void search_songs(void) {
    char query[MAX_LENGTH];
    printf("\n--- PRETRAGA PJESAMA ---\n");
    printf("Unesite naziv pjesme ili izvodjaca: ");
    scanf(" %99[^\n]", query);

    printf("\nRezultati pretrage:\n");
    bool found = false;

    char query_lower[MAX_LENGTH];
    strcpy(query_lower, query);
    to_lower_case(query_lower);

    for (int i = 0; i < num_songs; i++) {
        char title_lower[MAX_LENGTH];
        strcpy(title_lower, songs[i]->title);
        to_lower_case(title_lower);
        bool title_match = string_contains(title_lower, query_lower);

        bool artist_match = false;
        char artists_lower[MAX_LENGTH];
        strcpy(artists_lower, songs[i]->artist);
        to_lower_case(artists_lower);

        char* artist_token = strtok(artists_lower, ",");
        while (artist_token != NULL) {
            while (*artist_token == ' ') artist_token++;
            char* end = artist_token + strlen(artist_token) - 1;
            while (end > artist_token && *end == ' ') end--;
            *(end + 1) = '\0';

            if (strstr(artist_token, query_lower) != NULL) {
                artist_match = true;
                break;
            }
            artist_token = strtok(NULL, ",");
        }

        if (title_match || artist_match) {
            printf("%d. %s - %s (%d:%02d)\n",
                songs[i]->id,
                songs[i]->artist,
                songs[i]->title,
                songs[i]->duration / 60,
                songs[i]->duration % 60);
            found = true;
        }
    }

    if (!found) {
        printf("Nema rezultata za \"%s\"\n", query);
    }
}

bool string_contains(const char* str, const char* substr) {
    char str_lower[MAX_LENGTH];
    char substr_lower[MAX_LENGTH];
    char temp_str[MAX_LENGTH * 2];
    char word[MAX_LENGTH];

    strcpy(str_lower, str);
    strcpy(substr_lower, substr);
    to_lower_case(str_lower);
    to_lower_case(substr_lower);

    sprintf(temp_str, " %s ", str_lower);
    sprintf(word, " %s ", substr_lower);

    return strstr(temp_str, word) != NULL;
}

void load_songs_from_file(void) {
    FILE* file = fopen("songs.txt", "r");
    if (!file) {
        log_errno("fopen read songs.txt");
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        Song* s = (Song*)xmalloc(sizeof(*s));
        int read = sscanf(line, "%d|%99[^|]|%99[^|]|%99[^|]|%d|%d",
            &s->id, s->title, s->artist, s->album, &s->duration, &s->year);
        if (read != 6) {
            fprintf(stderr, "Upozorenje: preskacem neispravan red: %s", line);
            free(s);
            continue;
        }

        if (num_songs == songs_capacity) {
            songs_capacity *= 2;
            songs = (Song**)xrealloc(songs, (size_t)songs_capacity * sizeof(*songs));
        }
        songs[num_songs++] = s;
    }

    if (ferror(file)) {
        log_errno("fgets songs.txt");
    }
    else if (!feof(file)) {
        fprintf(stderr, "Upozorenje: čitanje prekinuto.\n");
    }
    if (fclose(file) == EOF) log_errno("fclose songs.txt");
}

static int cmp_ci(const char* a, const char* b) {
    unsigned char ca, cb;
    while (*a && *b) {
        ca = (unsigned char)tolower((unsigned char)*a++);
        cb = (unsigned char)tolower((unsigned char)*b++);
        if (ca != cb) return (int)ca - (int)cb;
    }
    ca = (unsigned char)tolower((unsigned char)*a);
    cb = (unsigned char)tolower((unsigned char)*b);
    return (int)ca - (int)cb;
}

void add_song_insert_sorted_by_title(void) {
    Song* ns = (Song*)xmalloc(sizeof(*ns));

    printf("\n--- DODAVANJE (INSERT po naslovu) ---\n");
    printf("Naziv pjesme: ");
    scanf(" %99[^\n]", ns->title);

    printf("Izvodjac: ");
    scanf(" %99[^\n]", ns->artist);

    printf("Album: ");
    scanf(" %99[^\n]", ns->album);

    printf("Trajanje (u sekundama): ");
    if (scanf("%d", &ns->duration) != 1) { puts("Neispravan unos."); free(ns); return; }

    printf("Godina izdanja: ");
    if (scanf("%d", &ns->year) != 1) { puts("Neispravan unos."); free(ns); return; }

    if (num_songs == songs_capacity) {
        songs_capacity *= 2;
        songs = (Song**)xrealloc(songs, (size_t)songs_capacity * sizeof(*songs));
    }

    int pos = num_songs;
    while (pos > 0 && cmp_ci(songs[pos - 1]->title, ns->title) > 0) {
        songs[pos] = songs[pos - 1];
        --pos;
    }
    songs[pos] = ns;
    ++num_songs;

    reorganize_song_ids();

    if (save_all_songs_to_file() == 0) {
        printf("Pjesma umetnuta na poziciju %d. ID: %d\n", pos + 1, ns->id);
    }
    else {
        puts("Upozorenje: doslo je do greske pri spremanju u datoteku.");
    }
}

static int cmp_song_id_key(const void* keyp, const void* elem) {
    const int id = *(const int*)keyp;
    const Song* const s = *(Song* const*)elem;
    return (id > s->id) - (id < s->id);
}

static int cmp_title_asc(const void* A, const void* B) {
    const Song* a = *(Song* const*)A;
    const Song* b = *(Song* const*)B;
    return cmp_ci(a->title, b->title);
}

int save_all_songs_to_file(void) {
    const char* path = "songs.txt";
    const char* tmp = "songs.tmp";

    {
        FILE* test = fopen(path, "rb");
        if (test) { fclose(test); copy_file(path, "songs.bak"); }
    }

    FILE* file = fopen(tmp, "w");
    if (!file) { perror("fopen songs.tmp"); return -1; }

    for (int i = 0; i < num_songs; i++) {
        if (fprintf(file, "%d|%s|%s|%s|%d|%d\n",
            songs[i]->id,
            songs[i]->title,
            songs[i]->artist,
            songs[i]->album,
            songs[i]->duration,
            songs[i]->year) < 0) {
            perror("fprintf songs.tmp");
            fclose(file);
            remove(tmp);
            return -2;
        }
    }
    if (fclose(file) == EOF) { perror("fclose songs.tmp"); remove(tmp); return -3; }

    if (remove(path) != 0 && errno != ENOENT) { perror("remove songs.txt"); remove(tmp); return -4; }
    if (rename(tmp, path) != 0) { perror("rename songs.tmp->songs.txt"); remove(tmp); return -5; }

    return 0;
}


long get_songs_file_size_bytes(void) {
    FILE* f = fopen("songs.txt", "rb");
    if (!f) return -1L;
    if (fseek(f, 0L, SEEK_END) != 0) { fclose(f); return -1L; }
    long size = ftell(f);
    rewind(f);
    fclose(f);
    return size;
}

static int cmp_artist_asc(const void* A, const void* B) {
    const Song* a = *(Song* const*)A;
    const Song* b = *(Song* const*)B;
    return cmp_ci(a->artist, b->artist);
}

static int cmp_year_asc(const void* A, const void* B) {
    const Song* a = *(Song* const*)A;
    const Song* b = *(Song* const*)B;
    return (a->year > b->year) - (a->year < b->year);
}

static int cmp_duration_asc(const void* A, const void* B) {
    const Song* a = *(Song* const*)A;
    const Song* b = *(Song* const*)B;
    return (a->duration > b->duration) - (a->duration < b->duration);
}

static void reverse_songs_array(void) {
    for (int i = 0, j = num_songs - 1; i < j; ++i, --j) {
        Song* tmp = songs[i];
        songs[i] = songs[j];
        songs[j] = tmp;
    }
}

typedef int (*cmp_fn)(const void*, const void*);

static void sort_songs_by_year_with_range(int ascending) {
    if (num_songs <= 1) {
        puts("Nema dovoljno pjesama za sortiranje.");
        return;
    }

    qsort(songs, num_songs, sizeof(Song*), cmp_year_asc);
    if (!ascending) reverse_songs_array();

    reorganize_song_ids();

    int min_year = 0, max_year = 0;
    get_year_range(&min_year, &max_year);

    if (save_all_songs_to_file() == 0) {
        printf("Pjesme su sortirane po godini (%d. do %d.) i spremljene u datoteku.\n",
            min_year, max_year);
    }
    else {
        puts("Upozorenje: doslo je do greske pri spremanju u datoteku.");
    }
}


static void sort_songs_by_duration_asc_simple(void) {
    if (num_songs <= 1) {
        puts("Nema dovoljno pjesama za sortiranje.");
        return;
    }
    qsort(songs, num_songs, sizeof(Song*), cmp_duration_asc);
    reorganize_song_ids();

    if (save_all_songs_to_file() == 0) {
        puts("Pjesme su sortirane po trajanju i spremljene u datoteku.");
    }
    else {
        puts("Upozorenje: doslo je do greske pri spremanju u datoteku.");
    }
}

void format_duration_mmss(int seconds, char out[8]) {
    Duration d;
    d.mmss.m = (unsigned short)(seconds / 60);
    d.mmss.s = (unsigned short)(seconds % 60);
    snprintf(out, 8, "%u:%02u", (unsigned)d.mmss.m, (unsigned)d.mmss.s);
}

int copy_file(const char* src, const char* dst) {
    FILE* in = fopen(src, "rb");
    if (!in) return -1;
    FILE* out = fopen(dst, "wb");
    if (!out) { fclose(in); return -2; }

    char buf[8192];
    size_t nread;
    while ((nread = fread(buf, 1, sizeof buf, in)) > 0) {
        if (fwrite(buf, 1, nread, out) != nread) {
            perror("fwrite");
            fclose(in); fclose(out);
            remove(dst);
            return -3;
        }
    }
    if (ferror(in)) { perror("fread"); fclose(in); fclose(out); remove(dst); return -4; }

    if (fclose(in) == EOF) { perror("fclose in");  fclose(out); remove(dst); return -5; }
    if (fclose(out) == EOF) { perror("fclose out"); remove(dst); return -6; }
    return 0;
}

Song* find_song_by_id_bsearch(int id) {
    if (id < 1 || id > num_songs) return NULL;
    Song** hit = (Song**)bsearch(&id, songs, (size_t)num_songs, sizeof(Song*), cmp_song_id_key);
    return hit ? *hit : NULL;
}

static Song* find_song_by_id_recursive_impl(int id, int lo, int hi) {
    if (lo > hi) return NULL;
    int mid = lo + (hi - lo) / 2;
    int mid_id = songs[mid]->id;
    if (id == mid_id) return songs[mid];
    if (id < mid_id)  return find_song_by_id_recursive_impl(id, lo, mid - 1);
    else              return find_song_by_id_recursive_impl(id, mid + 1, hi);
}

Song* find_song_by_id_recursive(int id) {
    if (id < 1 || id > num_songs) return NULL;
    return find_song_by_id_recursive_impl(id, 0, num_songs - 1);
}

void add_to_playlist(void) {
    if (num_playlists == 0) { printf("Prvo kreirajte playlistu!\n"); return; }

    printf("\n--- DODAVANJE U PLAYLISTU ---\nDostupne playliste:\n");
    for (int i = 0; i < num_playlists; i++) printf("%d. %s\n", i + 1, playlists[i].name);

    int playlist_choice;
    printf("Odabir playliste (1-%d): ", num_playlists);
    if (scanf("%d", &playlist_choice) != 1) { puts("Neispravan unos."); return; }
    playlist_choice--;
    if (playlist_choice < 0 || playlist_choice >= num_playlists) { puts("Neispravan odabir!"); return; }

    list_all_songs();
    int song_id;
    printf("Unesite ID pjesme za dodavanje: ");
    if (scanf("%d", &song_id) != 1) { puts("Neispravan ID."); return; }

    Song** hit = (Song**)bsearch(&song_id, songs, (size_t)num_songs, sizeof(Song*), cmp_song_id_key);
    if (!hit) { printf("Pjesma s ID %d nije pronadjena.\n", song_id); return; }

    Playlist* pl = &playlists[playlist_choice];
    if (pl->num_songs == pl->capacity) {
        pl->capacity = (pl->capacity == 0) ? INITIAL_CAPACITY : pl->capacity * 2;
        pl->songs = (Song**)xrealloc(pl->songs, (size_t)pl->capacity * sizeof(Song*));
    }
    pl->songs[pl->num_songs++] = *hit;
    puts("Pjesma dodana u playlistu!");
}

void add_song(void) {
    if (num_songs == songs_capacity) {
        songs_capacity *= 2;
        songs = (Song**)xrealloc(songs, (size_t)songs_capacity * sizeof(*songs));
    }

    Song* new_song = (Song*)xmalloc(sizeof(*new_song));

    printf("\n--- DODAVANJE PJESME ---\n");
    new_song->id = generate_song_id();

    printf("Naziv pjesme: ");
    scanf(" %99[^\n]", new_song->title);

    printf("Izvodjac: ");
    scanf(" %99[^\n]", new_song->artist);

    printf("Album: ");
    scanf(" %99[^\n]", new_song->album);

    printf("Trajanje (u sekundama): ");
    if (scanf("%d", &new_song->duration) != 1) {
        puts("Neispravan unos trajanja."); free(new_song); return;
    }

    printf("Godina izdanja: ");
    if (scanf("%d", &new_song->year) != 1) {
        puts("Neispravan unos godine."); free(new_song); return;
    }

    songs[num_songs++] = new_song;
    printf("Pjesma uspjesno dodana! (ID: %d)\n", new_song->id);

    append_song_to_file(new_song);
}
