#include <gtest/gtest.h>
#include "lib/rtttl.hpp"

using namespace rtttl;

// Temporary test literal - returns Track<N> directly
template<typename T, T... Ch>
constexpr auto operator""_test() {
    return rtttl::detail::rtttl_parser<Ch...>{}.parse();
}

// Test basic parsing functionality
TEST(RTTTLParserTest, BasicParsing) {
    constexpr auto track = "Nokia:d=4,o=5,b=125:8e6,8d6,f#,g#"_test;

    EXPECT_EQ(track.size(), 4u);
    EXPECT_EQ(track.default_duration, 4);
    EXPECT_EQ(track.default_octave, 5);
    EXPECT_EQ(track.bpm, 125);
}

// Test metadata parsing
TEST(RTTTLParserTest, MetadataParsing) {
    constexpr auto track1 = "Test:d=8,o=6,b=100:c"_test;
    EXPECT_EQ(track1.default_duration, 8);
    EXPECT_EQ(track1.default_octave, 6);
    EXPECT_EQ(track1.bpm, 100);

    constexpr auto track2 = "Test:d=16,o=4,b=200:c"_test;
    EXPECT_EQ(track2.default_duration, 16);
    EXPECT_EQ(track2.default_octave, 4);
    EXPECT_EQ(track2.bpm, 200);
}

// Test note parsing
TEST(RTTTLParserTest, NoteParsing) {
    constexpr auto track = "Test:d=4,o=5,b=120:c,d#,8e6,f.,p"_test;

    ASSERT_EQ(track.size(), 5u);

    // c (default duration and octave)
    EXPECT_EQ(track.notes[0].semitone, 72);  // C5
    EXPECT_EQ(track.notes[0].duration, 4);

    // d# (sharp note)
    EXPECT_EQ(track.notes[1].semitone, 75);  // D#5
    EXPECT_EQ(track.notes[1].duration, 4);

    // 8e6 (explicit duration and octave)
    EXPECT_EQ(track.notes[2].semitone, 88);  // E6
    EXPECT_EQ(track.notes[2].duration, 8);

    // f. (dotted note - 1.5x duration)
    EXPECT_EQ(track.notes[3].semitone, 77);  // F5
    EXPECT_EQ(track.notes[3].duration, 6);   // 4 + 4/2

    // p (pause)
    EXPECT_EQ(track.notes[4].semitone, 0);   // pause
    EXPECT_EQ(track.notes[4].duration, 4);
}

// Test semitone calculation
TEST(RTTTLParserTest, SemitoneCalculation) {
    constexpr auto track = "Test:d=4,o=4,b=120:c4,c#4,d4,d#4,e4,f4,f#4,g4,g#4,a4,a#4,b4,c5"_test;

    ASSERT_EQ(track.size(), 13u);

    // C4 = 60 in MIDI
    EXPECT_EQ(track.notes[0].semitone, 60);   // C4
    EXPECT_EQ(track.notes[1].semitone, 61);   // C#4
    EXPECT_EQ(track.notes[2].semitone, 62);   // D4
    EXPECT_EQ(track.notes[3].semitone, 63);   // D#4
    EXPECT_EQ(track.notes[4].semitone, 64);   // E4
    EXPECT_EQ(track.notes[5].semitone, 65);   // F4
    EXPECT_EQ(track.notes[6].semitone, 66);   // F#4
    EXPECT_EQ(track.notes[7].semitone, 67);   // G4
    EXPECT_EQ(track.notes[8].semitone, 68);   // G#4
    EXPECT_EQ(track.notes[9].semitone, 69);   // A4
    EXPECT_EQ(track.notes[10].semitone, 70);  // A#4
    EXPECT_EQ(track.notes[11].semitone, 71);  // B4
    EXPECT_EQ(track.notes[12].semitone, 72);  // C5
}

// Test different octaves
TEST(RTTTLParserTest, OctaveHandling) {
    constexpr auto track = "Test:d=4,o=5,b=120:c4,c5,c6,c7"_test;

    ASSERT_EQ(track.size(), 4u);

    EXPECT_EQ(track.notes[0].semitone, 60);  // C4
    EXPECT_EQ(track.notes[1].semitone, 72);  // C5
    EXPECT_EQ(track.notes[2].semitone, 84);  // C6
    EXPECT_EQ(track.notes[3].semitone, 96);  // C7
}

// Test direct parser usage
TEST(RTTTLParserTest, DirectParserUsage) {
    constexpr auto track = "Test:d=8,o=6,b=140:c,d,e"_test;

    EXPECT_EQ(track.size(), 3u);
    EXPECT_EQ(track.default_duration, 8);
    EXPECT_EQ(track.default_octave, 6);
    EXPECT_EQ(track.bpm, 140);
}

// Test real RTTTL songs
TEST(RTTTLParserTest, RealSongs) {
    constexpr auto mission = "MissionImp:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#"_test;
    EXPECT_EQ(mission.size(), 8u);
    EXPECT_EQ(mission.bpm, 95);
    EXPECT_EQ(mission.default_duration, 16);

    constexpr auto nokia = "Nokia:d=4,o=5,b=125:8e6,8d6,f#,g#,8c#6,8b,d,e,8b,8a,c#,e,2a"_test;
    EXPECT_EQ(nokia.size(), 13u);
    EXPECT_EQ(nokia.bpm, 125);
}

// This test verifies that everything works at compile-time
TEST(RTTTLParserTest, CompileTimeVerification) {
    constexpr auto track = "Test:d=4,o=5,b=120:c,d,e"_test;

    static_assert(track.size() == 3, "Track should have 3 notes");
    static_assert(track.default_duration == 4, "Default duration should be 4");
    static_assert(track.default_octave == 5, "Default octave should be 5");
    static_assert(track.bpm == 120, "BPM should be 120");

    static_assert(track.notes[0].semitone == 72, "First note should be C5 (72)");
    static_assert(track.notes[1].semitone == 74, "Second note should be D5 (74)");
    static_assert(track.notes[2].semitone == 76, "Third note should be E5 (76)");

    SUCCEED() << "All compile-time computations verified!";
}

// Test that different tracks have different types
TEST(RTTTLParserTest, TypeDifferences) {
    constexpr auto track1 = "Song1:d=4,o=5,b=120:c,d,e,f,g"_test;
    constexpr auto track2 = "Song2:d=8,o=6,b=140:a,b,c6"_test;

    // Different sized tracks should have different types
    static_assert(!std::is_same_v<decltype(track1), decltype(track2)>,
                  "Different sized tracks should have different types");

    EXPECT_EQ(track1.size(), 5u);
    EXPECT_EQ(track2.size(), 3u);
}

// Test empty and malformed strings
TEST(RTTTLParserTest, EdgeCases) {
    constexpr auto empty = "Empty:d=4,o=5,b=120:"_test;
    EXPECT_EQ(empty.size(), 0u);

    constexpr auto single = "Single:d=4,o=5,b=120:c"_test;
    EXPECT_EQ(single.size(), 1u);
    EXPECT_EQ(single.notes[0].semitone, 72);  // C5
}

// Performance/memory test - ensure no runtime overhead
TEST(RTTTLParserTest, NoRuntimeOverhead) {
    constexpr auto large = "Large:d=8,o=5,b=120:c,d,e,f,g,a,b,c6,d6,e6,f6,g6,a6,b6,c7,d7,e7,f7,g7,a7"_test;

    EXPECT_EQ(large.size(), 20u);

    static_assert(large.size() == 20, "Size should be computed at compile-time");
}

// Test conversion to tones with frequencies and durations in ms
TEST(RTTTLParserTest, ToTonesConversion) {
    constexpr auto track = "Test:d=4,o=4,b=120:c4,a4,8p"_test;
    constexpr auto tones = to_tones(track);

    // Check static conversion works
    static_assert(tones.size() == 3, "Should have 3 tones");

    // c4 = MIDI 60 = ~262 Hz, quarter note at 120 BPM = 500ms
    EXPECT_EQ(tones[0].freq_hz, 262);
    EXPECT_EQ(tones[0].duration_ms, 500);

    // a4 = MIDI 69 = 440 Hz (exact), quarter note = 500ms
    EXPECT_EQ(tones[1].freq_hz, 440);
    EXPECT_EQ(tones[1].duration_ms, 500);

    // pause = 0 Hz, eighth note = 250ms
    EXPECT_EQ(tones[2].freq_hz, 0);
    EXPECT_EQ(tones[2].duration_ms, 250);
}

// Test frequency accuracy across different octaves
TEST(RTTTLParserTest, FrequencyAccuracy) {
    constexpr auto track = "Test:d=4,o=5,b=120:c3,c4,c5,c6,c7"_test;
    constexpr auto tones = to_tones(track);

    // Test known frequencies (allowing ±1 Hz tolerance)
    EXPECT_NEAR(tones[0].freq_hz, 131, 1);   // C3 ≈ 130.81 Hz
    EXPECT_NEAR(tones[1].freq_hz, 262, 1);   // C4 ≈ 261.63 Hz
    EXPECT_NEAR(tones[2].freq_hz, 523, 1);   // C5 ≈ 523.25 Hz
    EXPECT_NEAR(tones[3].freq_hz, 1047, 1);  // C6 ≈ 1046.50 Hz
    EXPECT_NEAR(tones[4].freq_hz, 2093, 1);  // C7 ≈ 2093.00 Hz
}

// Test duration calculations with different BPM and note values
TEST(RTTTLParserTest, DurationCalculations) {
    // Test at 60 BPM for easy math: quarter note = 1000ms
    constexpr auto track = "Test:d=4,o=5,b=60:1c,2c,4c,8c,16c"_test;
    constexpr auto tones = to_tones(track);

    EXPECT_EQ(tones[0].duration_ms, 4000);  // whole note = 4000ms
    EXPECT_EQ(tones[1].duration_ms, 2000);  // half note = 2000ms
    EXPECT_EQ(tones[2].duration_ms, 1000);  // quarter note = 1000ms
    EXPECT_EQ(tones[3].duration_ms, 500);   // eighth note = 500ms
    EXPECT_EQ(tones[4].duration_ms, 250);   // sixteenth note = 250ms
}

// Test that tone conversion works at compile time
TEST(RTTTLParserTest, CompileTimeToneConversion) {
    constexpr auto track = "Test:d=4,o=5,b=120:c,d,e"_test;
    constexpr auto tones = to_tones(track);

    // All these static_assert prove compile-time conversion
    static_assert(tones.size() == 3, "Should have 3 tones");
    static_assert(tones[0].freq_hz == 523, "C5 should be 523 Hz");
    static_assert(tones[1].freq_hz == 587, "D5 should be 587 Hz");
    static_assert(tones[2].freq_hz == 659, "E5 should be 659 Hz");
    static_assert(tones[0].duration_ms == 500, "Quarter note at 120 BPM = 500ms");

    SUCCEED() << "All compile-time tone conversions verified!";
}

// Test the convenient _rtttl2tones literal
TEST(RTTTLParserTest, TonesLiteral) {
    constexpr auto tones = "Simple:d=4,o=5,b=60:c,g,8c6"_rtttl2tones;

    // Verify compile-time evaluation
    static_assert(tones.size == 3, "Should have 3 tones");
    static_assert(tones.data[0].freq_hz == 523, "C5 = 523 Hz");
    static_assert(tones.data[1].freq_hz == 784, "G5 = 784 Hz");
    static_assert(tones.data[2].freq_hz == 1047, "C6 = 1047 Hz");
    static_assert(tones.data[0].duration_ms == 1000, "Quarter note at 60 BPM = 1000ms");
    static_assert(tones.data[2].duration_ms == 500, "Eighth note at 60 BPM = 500ms");

    // Runtime checks for good measure
    EXPECT_EQ(tones.size, 3u);
    EXPECT_EQ(tones.data[0].freq_hz, 523);
    EXPECT_EQ(tones.data[0].duration_ms, 1000);
    EXPECT_EQ(tones.data[1].freq_hz, 784);
    EXPECT_EQ(tones.data[1].duration_ms, 1000);
    EXPECT_EQ(tones.data[2].freq_hz, 1047);
    EXPECT_EQ(tones.data[2].duration_ms, 500);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
