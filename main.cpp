#include <ctime>
#include <set>
#include <random>
#include <future>
#include <thread>
#include <unordered_set>
#include <span>
#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <assert.h>
#include <map>

#include <fmt/format.h>

/*------------------------------------------------------------------*/
// Timing:

using Clock = std::chrono::steady_clock;
using std::chrono::time_point;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using namespace std::literals::chrono_literals;

#define BEGIN_TIME_MEASURE time_point<Clock> start = Clock::now();

#define PRINT_TIME_MEASURE                                      \
time_point<Clock> end = Clock::now();                           \
nanoseconds diff = duration_cast<nanoseconds>(end - start);     \
std::cout << diff.count() << "ns" << std::endl;                 \

/*------------------------------------------------------------------*/
// Contains-helper:

template<typename T>
[[nodiscard]] inline bool contains(const std::vector<T>& vector, const T& t)
{
    return std::find(vector.cbegin(), vector.cend(), t) != vector.cend();
}

template<typename Key, typename AssociativeContainer>
[[nodiscard]] inline bool contains(const AssociativeContainer& container, const Key& key)
{
    return container.find(key) != container.cend();
};

/*------------------------------------------------------------------*/
// Threading:

template<typename T>
bool is_ready(const std::future<T>& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

/*------------------------------------------------------------------*/
// Skulls:

// Each skull scores points according to the instruction.
enum class Skull
{
    Void,

    Lover,
    Villager,
    Assassin,
    Priest,
    Royal,
    Guard,
    Hangman,

    Any
};

/*------------------------------------------------------------------*/
// Cards:

using ID = uint8_t;

// A vertical pair of Skulls.
struct Card
{
    Skull bottom;
    Skull top;

    ID id;
    auto operator <=>(const Card& other) { return id <=> other.id; }
};

std::map<char, Skull> SKULL_ENUM_MAP {
    { 'L', Skull::Lover },
    { 'V', Skull::Villager },
    { 'A', Skull::Assassin },
    { 'P', Skull::Priest },
    { 'R', Skull::Royal },
    { 'G', Skull::Guard },
    { 'H', Skull::Hangman },

    { '-', Skull::Void }
};

Skull get_skull_enum(const char ch)
{
    return SKULL_ENUM_MAP.at(ch);
}

char get_skull_char(const Skull sk)
{
    for (auto it = SKULL_ENUM_MAP.begin(); it != SKULL_ENUM_MAP.end(); ++it)
        if (it->second == sk)
            return it->first;

    assert(!"unknown enum");
    exit(EXIT_FAILURE);
}

// Creates a Card from a string, where:
// [0] indicates the first letter of the bottom Skull;
// [1] indicates the first letter of the top Skull;
Card create_card(const std::string skulls, const ID id)
{
    assert(skulls.length() == 2);

    return Card {
        .bottom = get_skull_enum(skulls[0]),
        .top    = get_skull_enum(skulls[1]),
        .id     = id,
    };
}

// Contains data for initializing the pool of all available cards. See create_card().
std::vector<std::string> cards_DATA {
    // Hangmen:
    "AH",
    "PH",
    "VH",
    "HA",
    "HR",
    "HL",

    // Guards:
    "GL",
    "GV",
    "GA",
    "GA",
    "GP",
    "GR",

    // Main:
    "PR",
    "AL",
    "AR",
    "PV",
    "RP",
    "LP",
    "VL",
    "AV",
    "AV",
    "RA",
    "LA",
    "PL",
    "PP",
    "AA",
    "LV",
    "VP",
    "VA",
    "VA",
};

/*------------------------------------------------------------------*/
// Pyramid:

// POI: Pyramids may have flat top rows, therefore they are defined by both their base width AND height.
struct Pyramid
{
    size_t base;
    size_t height;
    size_t size;
};

Pyramid create_pyramid_from_user_input()
{
    size_t pyramid_base, pyramid_height;

    do
    {
        std::cout << "Enter pyramid base (max 10):\n";
        std::cin >> pyramid_base;
    } while (pyramid_base > 10 || pyramid_base == 0);
    do
    {
        std::cout << "Enter pyramid height (max 10):\n";
        std::cin >> pyramid_height;
    } while (pyramid_height > 10 || pyramid_height == 0 || pyramid_height > pyramid_base);

    size_t pyramid_size = 0;
    for (size_t y = 0; y != pyramid_height; ++y)
        pyramid_size += pyramid_base - y;

    return Pyramid {
        .base   = pyramid_base,
        .height = pyramid_height,
        .size   = pyramid_size,
    };
}

/*------------------------------------------------------------------*/
// SkullGrid

/*
> A permutation of cards forms a pyramid by slotting n amount of cards from it into a growing pyramid, left-to-right & bottom-to-top.
The following is an example of a base-3, height-3 pyramid:
  5
 3 4
0 1 2
Where each number represents a Card from the vector.

> A Grid of Skulls is a simple 2D-vector of Skulls.
The following is an example of a base-3, height-3 Pyramid converted into a Grid:
5: - - S - -
4: - - S - -
3: - S - S -
2: - S - S -
1: S - S - S
0: S - S - S
   ^ ^ ^ ^ ^
   0 1 2 3 4
Where each 'S' represents some Skull from a Card and each '-' represents a void. Void is padding used to simplify card alignment.
*/

struct Coord
{
    size_t x;
    size_t y;

    bool operator==(const Coord& other) const
    {
        return x == other.x && y == other.y;
    }
};

namespace std
{
template<> struct hash<Coord>
{
    size_t operator()(const Coord& c) const
    {
        return (c.x << 32) | c.y;
    }
};
}

struct SkullGrid
{
    std::vector<Skull> grid;
    size_t width;
    size_t height;

    void print()
    {
        for (size_t y = 0; y != height; ++y)
        {
            size_t inverted_y = height - y - 1;

            for (size_t x = 0; x != width; ++x)
                std::cout << get_skull_char(grid[x + inverted_y * width]) << ' ';
            std::cout << '\n';
        }
    }

    int get_score() const
    {
        // Returns a map of <Coord, Skull> pairs around a central coordinate.
        // If specified, Skulls at certain coordinates will be excluded.
        auto get_adjacents =[&](const Coord coord, const std::unordered_set<Coord>& excluded = {}, const Skull skull_filter = Skull::Any)
        {
            std::vector<std::pair<Coord, Skull>> adjacents;
            adjacents.reserve(5);

            auto add_if_valid = [&](const Coord c)
            {
                if ((c.x < 0 || c.x >= width) || (c.y < 0 || c.y >= height) || contains(excluded, c))
                    return;

                Skull skull = grid[c.x + c.y * width];
                if (skull != Skull::Void && (skull_filter == Skull::Any || skull == skull_filter))
                    adjacents.emplace_back(std::make_pair(c, skull));
                return;
            };

            add_if_valid(Coord { coord.x - 2, coord.y + 0 }); // Left
            add_if_valid(Coord { coord.x + 2, coord.y + 0 }); // Right

            const bool even_level = coord.y % 2 == 0;
            if (even_level)
            {
                add_if_valid(Coord { coord.x + 0, coord.y + 1 }); // Top
                add_if_valid(Coord { coord.x - 1, coord.y - 1 }); // Bottom Left
                add_if_valid(Coord { coord.x + 1, coord.y - 1 }); // Bottom Right
            }
            else
            {
                add_if_valid(Coord { coord.x + 0, coord.y - 1 }); // Bottom
                add_if_valid(Coord { coord.x - 1, coord.y + 1 }); // Top Left
                add_if_valid(Coord { coord.x + 1, coord.y + 1 }); // Top right
            }

            return adjacents;
        };

        int lover_score = 0;
        int villager_score = 0;
        int assassin_score = 0;
        int priest_score = 0;
        int royal_score = 0;
        int guard_score = 0;
        int hangman_score = 0;

        std::unordered_set<Coord> excluded_lovers;
        excluded_lovers.reserve(6);

        std::unordered_set<size_t> excluded_priest_levels;
        excluded_lovers.reserve(10);

        int villagers_under_current_level = 0;
        int royals_under_current_level = 0;

        for (size_t y = 0; y != height; ++y)
        {
            int villagers_in_current_level = 0;
            int royals_in_current_level = 0;

            for (size_t x = (y / 2); x < width - (y / 2); x += 2)
            {
                const Coord coord { x, y };
                const Skull skull = grid[x + y * width];

                // Lovers:
                if (skull == Skull::Lover && !contains(excluded_lovers, coord))
                {
                    excluded_lovers.emplace(coord);

                    // Counts all connected lovers; adds any found lovers to excluded_lovers vector to avoid duplication.
                    auto recurse =[&](const Coord coord, auto& self_ref) -> int
                    {
                        auto adjacents = get_adjacents(coord, excluded_lovers, Skull::Lover);

                        int recursive_lovers = 0;
                        if (!adjacents.empty())
                        {
                            for (auto adj : adjacents)
                                excluded_lovers.emplace(std::get<Coord>(adj));
                            for (auto adj : adjacents)
                                recursive_lovers += self_ref(std::get<Coord>(adj), self_ref);
                        }

                        return static_cast<int>(adjacents.size()) + recursive_lovers;
                    };

                    // Score all connected lovers as one block (rounded down, odd lovers get nothing):
                    auto connected_lovers = 1 + recurse(coord, recurse);
                    lover_score += (connected_lovers / 2) * 6;
                }

                // Villagers:
                else if (skull == Skull::Villager)
                {
                    ++villager_score;
                    ++villagers_in_current_level;
                }

                // Assassins:
                else if (skull == Skull::Assassin)
                {
                    const auto adjacents = get_adjacents(coord, excluded_lovers);
                    for (const auto& [adj_coord, adj_skull] : adjacents)
                        if (adj_skull == Skull::Priest)
                        {
                            assassin_score += 2;
                            break;
                        }
                }

                // Priests:
                else if (skull == Skull::Priest && !contains(excluded_priest_levels, y))
                {
                    priest_score += 2;
                    excluded_priest_levels.emplace(y);
                }

                // Royals:
                else if (skull == Skull::Royal)
                {
                    royal_score += villagers_under_current_level + royals_under_current_level;
                    ++royals_in_current_level;
                }

                // Guards:
                else if (skull == Skull::Guard)
                    guard_score += 1 + royals_under_current_level;

                // Hangmen:
                else if (skull == Skull::Hangman)
                {
                    std::unordered_set<Coord> excluded_assassins;

                    // Counts all connected assassins; adds any found assassins to the excluded_assassins vector to avoid duplication.
                    auto recurse =[&](const Coord coord, auto& self_ref) -> int
                    {
                        auto adjacents = get_adjacents(coord, excluded_assassins, Skull::Assassin);

                        int recursive_assassins = 0;
                        if (!adjacents.empty())
                        {
                            for (auto adj : adjacents)
                                excluded_assassins.emplace(std::get<Coord>(adj));
                            for (auto adj : adjacents)
                                recursive_assassins += self_ref(std::get<Coord>(adj), self_ref);
                        }

                        return static_cast<int>(adjacents.size()) + recursive_assassins;

                    };

                    hangman_score += 1 + recurse(coord, recurse);
                }
            }

            villagers_under_current_level += villagers_in_current_level;
            royals_under_current_level += royals_in_current_level;
        }

        return lover_score + villager_score + assassin_score + priest_score + royal_score + guard_score + hangman_score;
    }
};

// Converts Cards into a "Grid of Skulls" for printing and scoring.
SkullGrid create_skull_grid(std::span<Card> cards, const Pyramid pyramid)
{
    size_t width  = pyramid.base * 2 - 1;
    size_t height = pyramid.height * 2;

    // Create Grid:
    std::vector<Skull> grid { width * height };

    // Fill Grid:
    size_t card_x = 0;
    size_t card_y = 0;
    for (size_t i = 0; i != pyramid.size; ++i)
    {
        // Convert Card coordinates (within Pyramid) to Skull coordinates (within Grid):
        const size_t skull_x = card_y + (card_x + 1) * 2 - 2;
        const size_t skull_bot_y = card_y * 2;
        const size_t skull_top_y = skull_bot_y + 1;

        // Assign Skull:
        grid[skull_x + skull_bot_y * width] = cards[i].bottom;
        grid[skull_x + skull_top_y * width] = cards[i].top;

        // Increment Card coordinate:
        if (++card_x == pyramid.base - card_y)
        {
            card_x = 0;
            ++card_y;
        }
    }

    return SkullGrid {
        .grid   = std::move(grid),
        .width  = std::move(width),
        .height = std::move(height),
    };
}

/*------------------------------------------------------------------*/
// Permutations:

struct Permutation
{
    int score;
    std::vector<Card> cards;
    Pyramid pyramid;

    void print(std::string prefix_message)
    {
        fmt::print("\n{}\nScore: {}\n", prefix_message, score);
        auto skull_grid = create_skull_grid(std::span(cards), pyramid);
        fmt::print("Permutation:\n");
        for (auto c : cards)
            fmt::print("{} ", c.id);
        fmt::print("\n");
        skull_grid.print();
    }

    void shuffle()
    {
        thread_local std::random_device rd;
        thread_local std::mt19937 mt(rd());

        std::shuffle(cards.begin(), cards.end(), mt);
        score = create_skull_grid(cards, pyramid).get_score();
    }

    void random_swap(const size_t swaps)
    {
        for (size_t i = 0; i != swaps; ++i)
        {
            size_t first, second;
            first = rand() % cards.size();
            second = rand() % cards.size();
            std::swap(cards[first], cards[second]);
        }
        score = create_skull_grid(cards, pyramid).get_score();
    }

    bool evolve_by_shuffle()
    {
        const size_t stall_indicator = 1'000'000;

        Permutation leader = *this;
        Permutation p = *this;

        size_t stall_counter = 0;
        while (true)
        {
            p.shuffle();
            if (p.score > leader.score)
            {
                leader = p;
                stall_counter = 0;
            }
            else
            {
                ++stall_counter;
                if (stall_counter >= stall_indicator)
                    break;
            }
        }

        if (leader.score > this->score)
        {
            *this = leader;
            return true;
        }
        else
            return false;
    }
    
    // Evolves by making random swaps. Keeps evolving until stalls.
    bool evolve_by_swap(const size_t swaps_per_evolve)
    {
        const size_t stall_indicator = 1'000'000;

        Permutation leader = *this;
        Permutation p;

        size_t stall_counter = 0;
        while (true)
        {
            p = leader;
            p.random_swap(swaps_per_evolve);

            if (p.score > leader.score)
            {
                leader = p;
                stall_counter = 0;
            }
            else
            {
                ++stall_counter;
                if (stall_counter >= stall_indicator)
                    break;
            }
        }

        if (leader.score > this->score)
        {
            *this = leader;
            return true;
        }
        else
            return false;
    }
};

Permutation create_permutation(std::vector<Card> cards, const Pyramid pyramid)
{
    return Permutation {
        .score = create_skull_grid(std::span(cards), pyramid).get_score(),
        .cards = std::move(cards),
        .pyramid = pyramid,
    };
}

std::vector<Permutation> create_leaderboard(
    const std::vector<Card> cards,
    const Pyramid pyramid,
    const size_t thread_count)
{
    Permutation p = create_permutation(cards, pyramid);

    // Create random leaderboard:
    std::vector<Permutation> leaderboard;
    for (size_t i = 0; i != thread_count; ++i)
    {
        p.shuffle();
        leaderboard.push_back(p);
    }

    // Evolve each permutation by shuffling it:
    std::vector<std::future<bool>> futures { thread_count };
    for (size_t i = 0; i != thread_count; ++i)
        futures[i] = std::async(std::launch::async, &Permutation::evolve_by_shuffle, &leaderboard[i]);

    for (size_t i = 0; i != thread_count; ++i)
        futures[i].wait();

    return leaderboard;
}

std::vector<Permutation> evolve_leaderboard(
    std::vector<Permutation> leaderboard,
    const size_t thread_count,
    const size_t swaps_per_evolve)
{
    // Evolve each permutation by shuffling it:
    std::vector<std::future<bool>> futures { thread_count };
    for (size_t i = 0; i != thread_count; ++i)
        futures[i] = std::async(std::launch::async, &Permutation::evolve_by_swap, &leaderboard[i], swaps_per_evolve);

    for (size_t i = 0; i != thread_count; ++i)
    {
        
    }
    
    for (size_t i = 0; i != thread_count; ++i)
        futures[i].wait();

    return leaderboard;
}

/*------------------------------------------------------------------*/
// Main:

int main()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Initialize card pool (all available cards):
    std::vector<Card> cards;
    cards.reserve(cards_DATA.size());
    for (size_t i = 0; i != cards_DATA.size(); ++i)
        cards.emplace_back(create_card(cards_DATA[i], static_cast<ID>(i)));
    std::sort(cards.begin(), cards.end());

    while (true)
    {
        // Initialize pyramid:
        const Pyramid pyramid = create_pyramid_from_user_input();
        if (cards.size() < pyramid.size)
        {
            std::cout << "Pyramid too big; not enough cards!\n";
            continue;
        }

        // Prepare threads
        const size_t thread_count = std::thread::hardware_concurrency();
        size_t available_threads = thread_count;
        std::vector<std::future<std::pair<int, std::vector<Card>>>> futures { thread_count };
        std::vector<std::pair<int, std::vector<Card>>> thread_local_leaders { thread_count };

        // Create leaderboard:
        std::vector<Permutation> leaderboard = create_leaderboard(cards, pyramid, thread_count);

        auto print_leaderboard =[&]()
        {
            for (size_t i = 0; i != leaderboard.size(); ++i)
                leaderboard[i].print(fmt::format("Thread #{}:\n", i));
        };
        
        fmt::print("Created Leaderboard:\n");
        print_leaderboard();

        fmt::print("Evolved leaderboard:\n");
        
        size_t swaps_per_evolve = 1;
        evolve_leaderboard(leaderboard, thread_count, swaps_per_evolve);
        print_leaderboard();
    }
}
