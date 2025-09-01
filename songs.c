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

static void* xmalloc(size_t n) {
    void* p = malloc(n);
    if (!p) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return p;
}
static void* xrealloc(void* ptr, size_t n) {
    void* p = realloc(ptr, n);
    if (!p) {
        perror("realloc");
        free(ptr);
        exit(EXIT_FAILURE);
    }
    return p;
}

static void remove_song_from_all_playlists(Song* dead) {
    for (int i = 0; i < num_playlists; ++i) {
        Playlist* pl = &playlists[i];
        int w = 0;
        for (int r = 0; r < pl->num_songs; ++r) {
            if (pl->songs[r] != dead) {
                pl->songs[w++] = pl->songs[r];
            }
        }
        pl->num_songs = w;
    }
}

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

void cleanup_library(void) {
    for (int i = 0; i < num_songs; i++) {
        free(songs[i]);
        songs[i] = NULL;
    }
    free(songs);
    songs = NULL;
    num_songs = 0;
    songs_capacity = 0;

    for (int i = 0; i < num_playlists; i++) {
        free(playlists[i].songs);
        playlists[i].songs = NULL;
        playlists[i].num_songs = 0;
        playlists[i].capacity = 0;
    }
    free(playlists);
    playlists = NULL;
    num_playlists = 0;
    playlists_capacity = 0;
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

void delete_song(int id) {
    int found_index = -1;
    Song** hit = (Song**)bsearch(&id, songs, (size_t)num_songs, sizeof(Song*), cmp_song_id_key);
    if (!hit) {
        printf("Pjesma s ID %d nije pronadjena.\n", id);
        return;
    }
    found_index = (int)(hit - songs);

    Song* victim = songs[found_index];

    remove_song_from_all_playlists(victim);

    free(victim);
    for (int i = found_index; i < num_songs - 1; i++) {
        songs[i] = songs[i + 1];
    }
    num_songs--;

    reorganize_song_ids();

    FILE * file = fopen("songs.txt", "w");
    if (!file) {
        perror("fopen write songs.txt");
        return;
        
    }
    for (int i = 0; i < num_songs; i++) {
        if (fprintf(file, "%d|%s|%s|%s|%d|%d\n",
            songs[i]->id,
            songs[i]->title,
            songs[i]->artist,
            songs[i]->album,
            songs[i]->duration,
            songs[i]->year) < 0) {
            perror("fprintf songs.txt");
            fclose(file);
            return;
            
        }
        
    }
    if (fclose(file) == EOF) perror("fclose songs.txt");
    
        printf("Pjesma s ID %d uspjesno obrisana!\n", id);
    if (save_all_songs_to_file() == 0) {
        printf("Pjesma s ID %d uspjesno obrisana!\n", id);
    }
    else {
        puts("Upozorenje: doslo je do greske pri spremanju u datoteku.");
    }
    if (save_all_songs_to_file() == 0) {
        printf("Pjesma s ID %d uspjesno obrisana!\n", id);
    }
    else {
        puts("Upozorenje: doslo je do greske pri spremanju u datoteku.");
    }

}

void reorganize_song_ids(void) {
    for (int i = 0; i < num_songs; i++) {
        songs[i]->id = i + 1;
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

void list_all_songs(void) {
    printf("\n--- SVE PJESME ---\n");
    for (int i = 0; i < num_songs; i++) {
        printf("%d. %s - %s [%s] (%d:%02d, %d)\n",
            songs[i]->id,
            songs[i]->title,
            songs[i]->artist,
            songs[i]->album,
            songs[i]->duration / 60,
            songs[i]->duration % 60,
            songs[i]->year);
    }
}

void create_playlist(void) {
    if (num_playlists == playlists_capacity) {
        playlists_capacity *= 2;
        playlists = (Playlist*)xrealloc(playlists, (size_t)playlists_capacity * sizeof(*playlists));
    }

    printf("\n--- KREIRANJE PLAYLISTE ---\n");
    printf("Naziv playliste: ");
    scanf(" %99[^\n]", playlists[num_playlists].name);

    playlists[num_playlists].num_songs = 0;
    playlists[num_playlists].capacity = INITIAL_CAPACITY;
    playlists[num_playlists].songs = (Song**)xmalloc((size_t)INITIAL_CAPACITY * sizeof(Song*));

    num_playlists++;
    printf("Playlista kreirana!\n");
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


void to_lower_case(char* str) {
    for (int i = 0; str[i]; i++) str[i] = (char)tolower((unsigned char)str[i]);
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

void append_song_to_file(Song* s) {
    FILE* file = fopen("songs.txt", "a");
    if (!file) {
        perror("fopen append songs.txt");
        return;
    }
    if (fprintf(file, "%d|%s|%s|%s|%d|%d\n",
        s->id, s->title, s->artist, s->album, s->duration, s->year) < 0) {
        perror("fprintf songs.txt");
        fclose(file);
        return;
    }
    if (fclose(file) == EOF) perror("fclose songs.txt");
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
        fprintf(stderr, "Upozorenje: čitanje prekinuto prije EOF-a.\n");
    }
    if (fclose(file) == EOF) log_errno("fclose songs.txt");
}

void edit_song(int id) {
    Song* song = find_song_by_id_recursive(id);
    if (!song) {
        printf("Pjesma s ID %d nije pronadjena.\n", id);
        return;
    }

    printf("\n--- UREDIVANJE PJESME ---\n");
    printf("Trenutni podaci:\n");
    printf("1. Naziv: %s\n", song->title);
    printf("2. Izvodjac: %s\n", song->artist);
    printf("3. Album: %s\n", song->album);
    printf("4. Trajanje: %d sekundi (%d:%02d)\n", song->duration, song->duration / 60, song->duration % 60);
    printf("5. Godina: %d\n", song->year);
    printf("0. Odustani\n");

    int choice;
    printf("\nSto zelite urediti? (0-5): ");
    if (scanf("%d", &choice) != 1) { puts("Neispravan unos."); return; }

    switch (choice) {
    case 1:
        printf("Novi naziv: ");
        scanf(" %99[^\n]", song->title);
        break;
    case 2:
        printf("Novi izvodjac: ");
        scanf(" %99[^\n]", song->artist);
        break;
    case 3:
        printf("Novi album: ");
        scanf(" %99[^\n]", song->album);
        break;
    case 4:
        printf("Novo trajanje (u sekundama): ");
        if (scanf("%d", &song->duration) != 1) { puts("Neispravan unos."); return; }
        break;
    case 5:
        printf("Nova godina: ");
        if (scanf("%d", &song->year) != 1) { puts("Neispravan unos."); return; }
        break;
    case 0:
        printf("Odustajem od uredivanja.\n");
        return;
    default:
        printf("Neispravan odabir!\n");
        return;
    }

    FILE* file = fopen("songs.txt", "w");
    if (!file) { perror("fopen write songs.txt"); return; }

    for (int i = 0; i < num_songs; i++) {
        if (fprintf(file, "%d|%s|%s|%s|%d|%d\n",
            songs[i]->id,
            songs[i]->title,
            songs[i]->artist,
            songs[i]->album,
            songs[i]->duration,
            songs[i]->year) < 0) {
            perror("fprintf songs.txt");
            fclose(file);
            return;
        }
    }
    if (fclose(file) == EOF) perror("fclose songs.txt");

    printf("\nPjesma uspjesno uredena!\n");
    printf("Azurirana pjesma:\n");
    printf("%d. %s - %s [%s] (%d:%02d, %d)\n",
        song->id,
        song->title,
        song->artist,
        song->album,
        song->duration / 60,
        song->duration % 60,
        song->year);
}

void list_playlists(void) {
    printf("\n--- PLAYLISTE ---\n");
    for (int i = 0; i < num_playlists; i++) {
        char pjesma_str[32];
        if (playlists[i].num_songs == 1) strcpy(pjesma_str, "1 pjesma");
        else if (playlists[i].num_songs >= 2 && playlists[i].num_songs <= 4)
            sprintf(pjesma_str, "%d pjesme", playlists[i].num_songs);
        else sprintf(pjesma_str, "%d pjesama", playlists[i].num_songs);

        printf("\n%d. %s (%s):\n", i + 1, playlists[i].name, pjesma_str);
        for (int j = 0; j < playlists[i].num_songs; j++) {
            printf("   - %s - %s (%d:%02d)\n",
                playlists[i].songs[j]->title,
                playlists[i].songs[j]->artist,
                playlists[i].songs[j]->duration / 60,
                playlists[i].songs[j]->duration % 60);
        }
    }
}

void list_artist_songs(const char* artist_name) {
    printf("\nPjesme izvodjaca \"%s\":\n", artist_name);
    bool found = false;

    char search_artist[MAX_LENGTH];
    strcpy(search_artist, artist_name);
    to_lower_case(search_artist);

    for (int i = 0; i < num_songs; i++) {
        char current_artist[MAX_LENGTH];
        strcpy(current_artist, songs[i]->artist);
        to_lower_case(current_artist);

        char* token = strtok(current_artist, ",");
        while (token != NULL) {
            while (*token == ' ') token++;
            char* end = token + strlen(token) - 1;
            while (end > token && *end == ' ') end--;
            *(end + 1) = '\0';

            if (strcmp(token, search_artist) == 0) {
                printf("- %s (%d:%02d) [%s, %d]\n",
                    songs[i]->title,
                    songs[i]->duration / 60,
                    songs[i]->duration % 60,
                    songs[i]->album,
                    songs[i]->year);
                found = true;
                break;
            }
            token = strtok(NULL, ",");
        }
    }

    if (!found) {
        printf("Nema pjesama za izvodjaca \"%s\"\n", artist_name);
    }
}

static void* xcalloc(size_t n, size_t sz) {
    void* p = calloc(n, sz);
    if (!p) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    return p;
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

void print_songs_file_info(void) {
    long sz = get_songs_file_size_bytes();
    if (sz < 0) puts("songs.txt nije pronadjen.");
    else        printf("songs.txt velicina: %ld bajtova\n", sz);
}


void sort_songs_by_title_asc(void) {
    if (num_songs <= 1) {
        puts("Nema dovoljno pjesama za sortiranje.");
        return;
    }

    qsort(songs, num_songs, sizeof(Song*), cmp_title_asc);
    reorganize_song_ids();

    if (save_all_songs_to_file() == 0) {
        puts("Pjesme su sortirane po naslovu i spremljene u datoteku.");
    }
    else {
        puts("Upozorenje: dogodila se greska pri spremanju u datoteku.");
    }
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

static void get_year_range(int* min_year, int* max_year) {
    if (num_songs == 0) { *min_year = 0; *max_year = 0; return; }
    int mn = songs[0]->year, mx = songs[0]->year;
    for (int i = 1; i < num_songs; ++i) {
        if (songs[i]->year < mn) mn = songs[i]->year;
        if (songs[i]->year > mx) mx = songs[i]->year;
    }
    *min_year = mn;
    *max_year = mx;
}

static void get_duration_range(int* min_dur, int* max_dur) {
    if (num_songs == 0) { *min_dur = 0; *max_dur = 0; return; }
    int mn = songs[0]->duration, mx = songs[0]->duration;
    for (int i = 1; i < num_songs; ++i) {
        if (songs[i]->duration < mn) mn = songs[i]->duration;
        if (songs[i]->duration > mx) mx = songs[i]->duration;
    }
    *min_dur = mn;
    *max_dur = mx;
}

void sort_songs_by_artist_asc(void) {
    if (num_songs <= 1) {
        puts("Nema dovoljno pjesama za sortiranje.");
        return;
    }
    qsort(songs, num_songs, sizeof(Song*), cmp_artist_asc);
    reorganize_song_ids();

    if (save_all_songs_to_file() == 0) {
        puts("Pjesme su sortirane po izvodjacu i spremljene u datoteku.");
    }
    else {
        puts("Upozorenje: dogodila se greska pri spremanju u datoteku.");
    }
}

static void reverse_songs_array(void) {
    for (int i = 0, j = num_songs - 1; i < j; ++i, --j) {
        Song* tmp = songs[i];
        songs[i] = songs[j];
        songs[j] = tmp;
    }
}

typedef int (*cmp_fn)(const void*, const void*);

static void sort_with(cmp_fn cmp, int ascending, const char* label) {
    if (num_songs <= 1) { puts("Nema dovoljno pjesama za sortiranje."); return; }

    qsort(songs, num_songs, sizeof(Song*), cmp);
    if (!ascending) reverse_songs_array();

    reorganize_song_ids();

    if (save_all_songs_to_file() == 0) {
        printf("Pjesme su sortirane po %s (%s) i spremljene u datoteku.\n",
            label, ascending ? "A do Z" : "Z do A");
    }
    else {
        puts("Upozorenje: doslo je do greske pri spremanju u datoteku.");
    }
}

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

void sort_songs_menu(void) {
    int field = 0;
    puts("\n--- SORTIRANJE (v3) ---");
    puts("1) Naslov");
    puts("2) Izvodjac");
    puts("3) Godina (najstarija do najnovije)");
    puts("4) Trajanje (najkraća do najduže)");
    printf("Odaberi polje (1-4): ");
    if (scanf("%d", &field) != 1 || field < 1 || field > 4) { puts("Neispravan odabir."); return; }

    if (field == 3) {                 
        sort_songs_by_year_with_range(1);
        return;
    }
    if (field == 4) {                   
        sort_songs_by_duration_asc_simple();
        return;
    }

    int asc = 1;
    printf("Poredak (1=Uzlazno, 0=Silazno): ");
    if (scanf("%d", &asc) != 1 || (asc != 0 && asc != 1)) { puts("Neispravan odabir poretka."); return; }

    switch (field) {
    case 1: sort_with(cmp_title_asc, asc, "naslovu");   break;
    case 2: sort_with(cmp_artist_asc, asc, "izvodjacu"); break;
    default: puts("Nepoznata opcija."); break;
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

void print_library_stats(void) {
    if (num_songs == 0) {
        puts("Biblioteka je prazna.");
        return;
    }

    int total_sec = 0, min_sec = songs[0]->duration, max_sec = songs[0]->duration;
    for (int i = 0; i < num_songs; ++i) {
        int d = songs[i]->duration;
        total_sec += d;
        if (d < min_sec) min_sec = d;
        if (d > max_sec) max_sec = d;
    }

    double avg_sec = (double)total_sec / (double)num_songs;
    double avg_min = avg_sec / 60.0;        
    double pct_long = 0.0;
    int longer = 0;
    for (int i = 0; i < num_songs; ++i) if (songs[i]->duration > 210) ++longer;
    pct_long = (double)longer * 100.0 / (double)num_songs;

    printf("\n--- STATISTIKA ---\n");
    printf("Ukupno pjesama: %d\n", num_songs);
    printf("Prosjecno trajanje: %.2f s (%.2f min)\n", avg_sec, avg_min);
    printf("Najkraca / najduza: %d:%02d / %d:%02d\n",
        min_sec / 60, min_sec % 60, max_sec / 60, max_sec % 60);
    printf("Duljih od 3.5 min: %d (%.2f%%)\n", longer, pct_long);
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
