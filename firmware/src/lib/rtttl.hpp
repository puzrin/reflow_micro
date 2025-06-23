#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace rtttl {

// ======================== Data Structures ========================

struct Note {
    uint8_t semitone;     // 0=pause, otherwise semitone number (C4=60)
    uint8_t duration;     // duration in note values (1,2,4,8,16,32)
};

struct Tone {
    uint16_t freq_hz;     // frequency in Hz, 0=pause
    uint16_t duration_ms; // duration in milliseconds
};

template<size_t N>
struct Track {
    uint8_t default_duration;  // 1,2,4,8,16,32
    uint8_t default_octave;    // 4,5,6,7
    uint16_t bpm;              // beats per minute
    std::array<Note, N> notes;

    constexpr size_t size() const { return N; }
    constexpr const Note* data() const { return notes.data(); }
};

// ======================== Frequency Conversion ========================

// Lookup table for one octave (compile-time only, won't be stored in flash)
constexpr std::array<float, 12> semitone_ratios = {
    1.0f,        // C  (0 semitones)
    1.059463f,   // C# (1 semitone)
    1.122462f,   // D  (2 semitones)
    1.189207f,   // D# (3 semitones)
    1.259921f,   // E  (4 semitones)
    1.334840f,   // F  (5 semitones)
    1.414214f,   // F# (6 semitones)
    1.498307f,   // G  (7 semitones)
    1.587401f,   // G# (8 semitones)
    1.681793f,   // A  (9 semitones)
    1.781797f,   // A# (10 semitones)
    1.887749f    // B  (11 semitones)
};

constexpr uint16_t semitone_to_freq(uint8_t semitone) {
    if (semitone == 0) return 0;  // pause

    // MIDI note 69 = A4 = 440Hz
    int offset = int(semitone) - 69;
    int octave_offset = offset / 12;
    int semitone_offset = offset % 12;

    if (semitone_offset < 0) {
        semitone_offset += 12;
        octave_offset--;
    }

    float base_freq = 440.0f * semitone_ratios[semitone_offset];

    // Octave shift (*2 or /2)
    if (octave_offset > 0) {
        base_freq *= (1 << octave_offset);  // *2^n
    } else if (octave_offset < 0) {
        base_freq /= (1 << (-octave_offset));  // /2^n
    }

    return static_cast<uint16_t>(base_freq + 0.5f);
}

constexpr uint32_t duration_to_ms(uint8_t duration, uint16_t bpm) {
    // RTTTL: whole note = 4 beats, quarter note = 1 beat
    // ms = (60000 * 4) / (bpm * duration_value)
    return (60000UL * 4) / (bpm * duration);
}

template<size_t N>
constexpr auto to_tones(const Track<N>& track) {
    std::array<Tone, N> tones{};

    for (size_t i = 0; i < N; ++i) {
        tones[i].freq_hz = semitone_to_freq(track.notes[i].semitone);
        tones[i].duration_ms = static_cast<uint16_t>(duration_to_ms(track.notes[i].duration, track.bpm));
    }

    return tones;
}

// ======================== Implementation Details ========================

namespace detail {

constexpr bool is_digit(char c) { return c >= '0' && c <= '9'; }
constexpr bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
constexpr char to_lower(char c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

constexpr uint8_t note_char_to_semitone(char c) {
    switch (to_lower(c)) {
        case 'c': return 0;
        case 'd': return 2;
        case 'e': return 4;
        case 'f': return 5;
        case 'g': return 7;
        case 'a': return 9;
        case 'b': return 11;
        default: return 0xFF;
    }
}

constexpr uint8_t char_to_semitone(char note_char, uint8_t octave, bool sharp = false) {
    if (to_lower(note_char) == 'p') return 0;

    uint8_t semitone = note_char_to_semitone(note_char);
    if (semitone == 0xFF) return 0;

    return (octave + 1) * 12 + semitone + (sharp ? 1 : 0);
}

template<char... Ch>
struct rtttl_parser {
    static constexpr std::array<char, sizeof...(Ch)> data{Ch...};
    static constexpr std::string_view S{data.data(), sizeof...(Ch)};

    static constexpr auto parse() {
        constexpr size_t N = count_notes();
        Track<N> track{4, 6, 63, {}};  // defaults

        parse_metadata(track);

        auto it = find_notes_start();
        size_t note_idx = 0;

        while (it != S.end() && note_idx < N) {
            while (it != S.end() && (*it == ',' || *it == ' ')) ++it;
            if (it == S.end()) break;

            track.notes[note_idx] = parse_note(it, track);
            note_idx++;
        }

        return track;
    }

private:
    static constexpr size_t count_notes() {
        auto it = find_notes_start();
        if (it == S.end()) return 0;

        size_t count = 0;
        bool in_note = false;

        while (it != S.end()) {
            if (*it == ',') {
                if (in_note) count++;
                in_note = false;
            } else if (is_digit(*it) || is_alpha(*it) || *it == '#' || *it == '.') {
                in_note = true;
            }
            ++it;
        }
        if (in_note) count++;

        return count;
    }

    static constexpr auto find_notes_start() {
        auto it = S.begin();
        int colons = 0;

        while (it != S.end()) {
            if (*it == ':') {
                colons++;
                if (colons == 2) {
                    ++it;
                    break;
                }
            }
            ++it;
        }

        return it;
    }

    template<size_t N>
    static constexpr void parse_metadata(Track<N>& track) {
        auto it = S.begin();

        while (it != S.end() && *it != ':') ++it;
        if (it == S.end()) return;
        ++it;

        while (it != S.end() && *it != ':') {
            if (*it == 'd' && (it + 1) != S.end() && *(it + 1) == '=') {
                it += 2;
                uint8_t dur = 0;
                while (it != S.end() && is_digit(*it)) {
                    dur = dur * 10 + (*it - '0');
                    ++it;
                }
                track.default_duration = dur;
            } else if (*it == 'o' && (it + 1) != S.end() && *(it + 1) == '=') {
                it += 2;
                if (it != S.end() && is_digit(*it)) {
                    track.default_octave = *it - '0';
                    ++it;
                }
            } else if (*it == 'b' && (it + 1) != S.end() && *(it + 1) == '=') {
                it += 2;
                uint16_t bpm = 0;
                while (it != S.end() && is_digit(*it)) {
                    bpm = bpm * 10 + (*it - '0');
                    ++it;
                }
                track.bpm = bpm;
            } else {
                ++it;
            }
        }
    }

    template<size_t N, typename Iterator>
    static constexpr Note parse_note(Iterator& it, const Track<N>& track) {
        Note note{0, track.default_duration};

        if (it != S.end() && is_digit(*it)) {
            uint8_t dur = 0;
            while (it != S.end() && is_digit(*it)) {
                dur = dur * 10 + (*it - '0');
                ++it;
            }
            note.duration = dur;
        }

        if (it != S.end() && is_alpha(*it)) {
            char note_char = *it++;

            bool sharp = false;
            if (it != S.end() && *it == '#') {
                sharp = true;
                ++it;
            }

            uint8_t octave = track.default_octave;
            if (it != S.end() && is_digit(*it)) {
                octave = *it - '0';
                ++it;
            }

            if (it != S.end() && *it == '.') {
                note.duration = note.duration + note.duration / 2;
                ++it;
            }

            note.semitone = char_to_semitone(note_char, octave, sharp);
        }

        return note;
    }
};

} // namespace detail

} // namespace rtttl

// ======================== User-Defined Literals ========================

// Convenient literal - returns array<Tone, N> with frequencies and durations
template<typename T, T... Ch>
constexpr auto operator""_rtttl2tones() {
    constexpr auto track = rtttl::detail::rtttl_parser<Ch...>{}.parse();
    return rtttl::to_tones(track);
}
