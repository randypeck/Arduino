// PINBALL_AUDIO_TRACKS.H Rev: 01/21/26
// Audio track definitions for Screamo pinball
// Defines COM (voice), SFX, and MUS (music) track arrays

#ifndef PINBALL_AUDIO_TRACKS_H
#define PINBALL_AUDIO_TRACKS_H

#include <Arduino.h>

// ******************************************
// ***** AUDIO TRACK STRUCTS AND CONSTS *****
// ******************************************

// NOTE: Audio priority and flag constants are defined in Pinball_Consts.h:
// - AUDIO_PRIORITY_LOW, AUDIO_PRIORITY_MED, AUDIO_PRIORITY_HIGH
// - AUDIO_FLAG_NONE, AUDIO_FLAG_LOOP

// *** STRUCTURE DEFINITIONS ***

// COM (Voice/Speech) tracks - need length for ducking timing, priority for preemption
struct AudioComTrackDef {
  unsigned int trackNum;
  byte         lengthTenths;  // 0.1s units (16 = 1.6s, max 255 = 25.5s)
  byte         priority;      // AUDIO_PRIORITY_LOW/MED/HIGH
};

// SFX tracks - no length needed (except special cases handled separately)
struct AudioSfxTrackDef {
  unsigned int trackNum;
  byte         flags;  // AUDIO_FLAG_LOOP, etc.
};

// MUS (Music) tracks - need length for sequencing to next song
struct AudioMusTrackDef {
  unsigned int trackNum;
  byte         lengthSeconds;  // seconds (max 255 = 4m15s)
};

// *** COM TRACK ARRAYS ***
// Organized by category for easy maintenance

// DIAG COM tracks (101-102, 111-117)
const AudioComTrackDef comTracksDiag[] = {
  { 101, 15, AUDIO_PRIORITY_MED },   // Entering diagnostics
  { 102, 17, AUDIO_PRIORITY_MED },   // Exiting diagnostics
  { 111,  6, AUDIO_PRIORITY_MED },   // Lamps
  { 112,  7, AUDIO_PRIORITY_MED },   // Switches
  { 113,  7, AUDIO_PRIORITY_MED },   // Coils
  { 114,  7, AUDIO_PRIORITY_MED },   // Motors
  { 115,  7, AUDIO_PRIORITY_MED },   // Music
  { 116, 12, AUDIO_PRIORITY_MED },   // Sound Effects
  { 117,  7, AUDIO_PRIORITY_MED }    // Comments
};
const byte NUM_COM_DIAG = sizeof(comTracksDiag) / sizeof(comTracksDiag[0]);

// TILT WARNING COM tracks (201-205)
const AudioComTrackDef comTracksTiltWarning[] = {
  { 201, 26, AUDIO_PRIORITY_HIGH },  // Shake it again...
  { 202, 15, AUDIO_PRIORITY_HIGH },  // Easy there, Hercules
  { 203, 21, AUDIO_PRIORITY_HIGH },  // Whoa there, King Kong
  { 204,  6, AUDIO_PRIORITY_HIGH },  // Careful
  { 205,  6, AUDIO_PRIORITY_HIGH }   // Watch it
};
const byte NUM_COM_TILT_WARNING = sizeof(comTracksTiltWarning) / sizeof(comTracksTiltWarning[0]);

// TILT COM tracks (212-216)
const AudioComTrackDef comTracksTilt[] = {
  { 212, 28, AUDIO_PRIORITY_HIGH },  // Nice goin Hercules...
  { 213, 32, AUDIO_PRIORITY_HIGH },  // Congratulations King Kong...
  { 214, 19, AUDIO_PRIORITY_HIGH },  // Ya broke it ya big palooka
  { 215, 27, AUDIO_PRIORITY_HIGH },  // This aint the bumper cars...
  { 216, 29, AUDIO_PRIORITY_HIGH }   // Tilt! Thats what ya get...
};
const byte NUM_COM_TILT = sizeof(comTracksTilt) / sizeof(comTracksTilt[0]);

// BALL MISSING COM tracks (301-304)
const AudioComTrackDef comTracksBallMissing[] = {
  { 301, 55, AUDIO_PRIORITY_MED },   // Theres a ball missing...
  { 302, 53, AUDIO_PRIORITY_MED },   // Press the ball lift rod...
  { 303, 46, AUDIO_PRIORITY_MED },   // Shoot any balls...
  { 304, 41, AUDIO_PRIORITY_MED }    // Theres still a ball missing...
};
const byte NUM_COM_BALL_MISSING = sizeof(comTracksBallMissing) / sizeof(comTracksBallMissing[0]);

// START REJECT COM tracks (312-330)
const AudioComTrackDef comTracksStartReject[] = {
  { 312, 36, AUDIO_PRIORITY_MED },   // No free admission today...
  { 313, 19, AUDIO_PRIORITY_MED },   // Go ask yer mommy...
  { 314, 19, AUDIO_PRIORITY_MED },   // I told yas ta SCRAM
  { 315, 18, AUDIO_PRIORITY_MED },   // Ya aint gettin in...
  { 316, 19, AUDIO_PRIORITY_MED },   // Any o yous kids got a cigarette
  { 317, 19, AUDIO_PRIORITY_MED },   // Go shake down da couch...
  { 318, 28, AUDIO_PRIORITY_MED },   // When Is a kid...
  { 319, 26, AUDIO_PRIORITY_MED },   // This aint a charity...
  { 320, 25, AUDIO_PRIORITY_MED },   // Whadda I look like...
  { 321, 27, AUDIO_PRIORITY_MED },   // No ticket, no tumblin...
  { 322, 30, AUDIO_PRIORITY_MED },   // Quit pressin buttons...
  { 323, 33, AUDIO_PRIORITY_MED },   // Im losin money...
  { 324, 21, AUDIO_PRIORITY_MED },   // Ya broke, go sell a balloon
  { 325, 24, AUDIO_PRIORITY_MED },   // Step right up, after ya pay
  { 326, 30, AUDIO_PRIORITY_MED },   // Dis aint da soup kitchen...
  { 327, 24, AUDIO_PRIORITY_MED },   // Beat it kid...
  { 328, 21, AUDIO_PRIORITY_MED },   // Yas tryin ta sneak in...
  { 329, 26, AUDIO_PRIORITY_MED },   // Come back when yas got...
  { 330, 21, AUDIO_PRIORITY_MED }    // No coin, no joyride
};
const byte NUM_COM_START_REJECT = sizeof(comTracksStartReject) / sizeof(comTracksStartReject[0]);

// START COM tracks (351-357, 402)
const AudioComTrackDef comTracksStart[] = {
  { 351, 17, AUDIO_PRIORITY_MED },   // Okay kid, youre in
  { 352, 46, AUDIO_PRIORITY_MED },   // Press start again for a wilder ride...
  { 353, 36, AUDIO_PRIORITY_MED },   // Keep pressin Start...
  { 354, 19, AUDIO_PRIORITY_MED },   // Second guest, cmon in
  { 355, 18, AUDIO_PRIORITY_MED },   // Third guest, youre in
  { 356, 22, AUDIO_PRIORITY_MED },   // Fourth guest, through the turnstile
  { 357, 25, AUDIO_PRIORITY_MED },   // The parks full...
  { 402, 35, AUDIO_PRIORITY_MED }    // Hey Gang, Lets Ride the Screamo
};
const byte NUM_COM_START = sizeof(comTracksStart) / sizeof(comTracksStart[0]);

// PLAYER COM tracks (451-454)
const AudioComTrackDef comTracksPlayer[] = {
  { 451, 11, AUDIO_PRIORITY_MED },   // Guest 1
  { 452, 13, AUDIO_PRIORITY_MED },   // Guest 2
  { 453, 13, AUDIO_PRIORITY_MED },   // Guest 3
  { 454, 13, AUDIO_PRIORITY_MED }    // Guest 4
};
const byte NUM_COM_PLAYER = sizeof(comTracksPlayer) / sizeof(comTracksPlayer[0]);

// BALL COM tracks (461-465)
const AudioComTrackDef comTracksBall[] = {
  { 461, 11, AUDIO_PRIORITY_MED },   // Ball 1
  { 462, 10, AUDIO_PRIORITY_MED },   // Ball 2
  { 463, 11, AUDIO_PRIORITY_MED },   // Ball 3
  { 464, 12, AUDIO_PRIORITY_MED },   // Ball 4
  { 465, 12, AUDIO_PRIORITY_MED }    // Ball 5
};
const byte NUM_COM_BALL = sizeof(comTracksBall) / sizeof(comTracksBall[0]);

// BALL 1 COMMENT COM tracks (511-519) - for P2-P4 first ball
const AudioComTrackDef comTracksBall1Comment[] = {
  { 511, 11, AUDIO_PRIORITY_LOW },   // Climb aboard
  { 512, 14, AUDIO_PRIORITY_LOW },   // Explore the park
  { 513, 14, AUDIO_PRIORITY_LOW },   // Fire away
  { 514,  9, AUDIO_PRIORITY_LOW },   // Launch it
  { 515, 17, AUDIO_PRIORITY_LOW },   // Lets find a ride
  { 516, 18, AUDIO_PRIORITY_LOW },   // Show em how its done
  { 517,  8, AUDIO_PRIORITY_LOW },   // Youre up
  { 518, 16, AUDIO_PRIORITY_LOW },   // Your turn to ride
  { 519, 13, AUDIO_PRIORITY_LOW }    // Lets ride
};
const byte NUM_COM_BALL1_COMMENT = sizeof(comTracksBall1Comment) / sizeof(comTracksBall1Comment[0]);

// BALL 5 COMMENT COM tracks (531-540)
const AudioComTrackDef comTracksBall5Comment[] = {
  { 531, 18, AUDIO_PRIORITY_LOW },   // Dont embarrass yourself
  { 532, 16, AUDIO_PRIORITY_LOW },   // Its now or never
  { 533, 17, AUDIO_PRIORITY_LOW },   // Last ride of the day
  { 534, 12, AUDIO_PRIORITY_LOW },   // Make it flashy
  { 535, 10, AUDIO_PRIORITY_LOW },   // No pressure
  { 536, 17, AUDIO_PRIORITY_LOW },   // This ball decides it
  { 537, 17, AUDIO_PRIORITY_LOW },   // This is it
  { 538, 15, AUDIO_PRIORITY_LOW },   // This is your last ticket
  { 539, 11, AUDIO_PRIORITY_LOW },   // Last ball
  { 540, 11, AUDIO_PRIORITY_LOW }    // Make it count
};
const byte NUM_COM_BALL5_COMMENT = sizeof(comTracksBall5Comment) / sizeof(comTracksBall5Comment[0]);

// GAME OVER COM tracks (551-577)
const AudioComTrackDef comTracksGameOver[] = {
  { 551, 17, AUDIO_PRIORITY_MED },   // End of the ride, thrillseeker
  { 552, 25, AUDIO_PRIORITY_MED },   // Hope you enjoyed the ride...
  { 553, 15, AUDIO_PRIORITY_MED },   // Its curtains for you, pal
  { 554, 19, AUDIO_PRIORITY_MED },   // Its game over for you, dude
  { 555, 30, AUDIO_PRIORITY_MED },   // No more rides for you...
  { 556, 17, AUDIO_PRIORITY_MED },   // No more tickets for this ride
  { 557, 20, AUDIO_PRIORITY_MED },   // Pack it up...
  { 558, 47, AUDIO_PRIORITY_MED },   // Randy warned me...
  { 559, 17, AUDIO_PRIORITY_MED },   // Rides over, move along
  { 560, 35, AUDIO_PRIORITY_MED },   // Screamo is now closed, but please...
  { 561, 31, AUDIO_PRIORITY_MED },   // Step right down...
  { 562, 18, AUDIO_PRIORITY_MED },   // Thats all she wrote...
  { 563, 33, AUDIO_PRIORITY_MED },   // The fat lady has sung...
  { 564, 22, AUDIO_PRIORITY_MED },   // Screamo is now closed, pal
  { 565, 15, AUDIO_PRIORITY_MED },   // Youre out of the running...
  { 566, 20, AUDIO_PRIORITY_MED },   // The shows over for you...
  { 567, 16, AUDIO_PRIORITY_MED },   // Youre out of tickets...
  { 568, 40, AUDIO_PRIORITY_MED },   // Thats the end of the line...
  { 569, 36, AUDIO_PRIORITY_MED },   // The Park is Now Closed...
  { 570, 34, AUDIO_PRIORITY_MED },   // Screamo is parked...
  { 571, 27, AUDIO_PRIORITY_MED },   // Youve hit the brakes...
  { 572, 39, AUDIO_PRIORITY_MED },   // Youve reached the end...
  { 573, 28, AUDIO_PRIORITY_MED },   // You gave it a whirl...
  { 574, 32, AUDIO_PRIORITY_MED },   // Your Ride is Over Park Now Closed
  { 575, 26, AUDIO_PRIORITY_MED },   // Your tickets punched...
  { 576, 42, AUDIO_PRIORITY_MED },   // Your Ride is Over Safety Bar
  { 577, 27, AUDIO_PRIORITY_MED }    // Your ride is over; better luck...
};
const byte NUM_COM_GAME_OVER = sizeof(comTracksGameOver) / sizeof(comTracksGameOver[0]);

// SHOOT COM tracks (611-620)
const AudioComTrackDef comTracksShoot[] = {
  { 611, 37, AUDIO_PRIORITY_LOW },   // Press the Ball Lift rod...
  { 612, 28, AUDIO_PRIORITY_LOW },   // This ride will be a lot more fun...
  { 613, 26, AUDIO_PRIORITY_LOW },   // I recommend you consider...
  { 614, 28, AUDIO_PRIORITY_LOW },   // Dont be afraid of the ride...
  { 615, 23, AUDIO_PRIORITY_LOW },   // For Gods sake shoot the ball
  { 616, 25, AUDIO_PRIORITY_LOW },   // No dilly dallying...
  { 617, 25, AUDIO_PRIORITY_LOW },   // Now would be a good time...
  { 618, 29, AUDIO_PRIORITY_LOW },   // Now would be an excellent time...
  { 619, 13, AUDIO_PRIORITY_LOW },   // Shoot the Ball
  { 620, 35, AUDIO_PRIORITY_LOW }    // This would be an excellent time... (annoyed)
};
const byte NUM_COM_SHOOT = sizeof(comTracksShoot) / sizeof(comTracksShoot[0]);

// BALL SAVED COM tracks (631-636, 641)
const AudioComTrackDef comTracksBallSaved[] = {
  { 631, 25, AUDIO_PRIORITY_MED },   // Ball saved, launch again
  { 632, 24, AUDIO_PRIORITY_MED },   // Heres another ball keep shooting
  { 633, 20, AUDIO_PRIORITY_MED },   // Keep going shoot again
  { 634, 21, AUDIO_PRIORITY_MED },   // Ball saved, ride again
  { 635, 18, AUDIO_PRIORITY_MED },   // Ball saved, shoot again
  { 636, 25, AUDIO_PRIORITY_MED },   // Ball saved; get back on the ride
  { 641, 25, AUDIO_PRIORITY_MED }    // Heres your ball back... (mode end)
};
const byte NUM_COM_BALL_SAVED = sizeof(comTracksBallSaved) / sizeof(comTracksBallSaved[0]);

// BALL SAVED URGENT COM tracks (651-662)
const AudioComTrackDef comTracksBallSavedUrgent[] = {
  { 651, 20, AUDIO_PRIORITY_MED },   // Heres another ball; send it
  { 652, 25, AUDIO_PRIORITY_MED },   // Heres another ball; fire at will
  { 653, 22, AUDIO_PRIORITY_MED },   // Heres a new ball; fire away
  { 654, 21, AUDIO_PRIORITY_MED },   // Heres a new ball; quick, shoot it
  { 655, 21, AUDIO_PRIORITY_MED },   // Heres another ball; shoot it now
  { 656, 18, AUDIO_PRIORITY_MED },   // Hurry, shoot another ball
  { 657, 15, AUDIO_PRIORITY_MED },   // Quick, shoot again
  { 658, 21, AUDIO_PRIORITY_MED },   // Keep going; shoot another ball
  { 659, 26, AUDIO_PRIORITY_MED },   // Its still your turn; keep shooting
  { 660, 10, AUDIO_PRIORITY_MED },   // Shoot again
  { 661, 24, AUDIO_PRIORITY_MED },   // For Gods sake shoot another ball
  { 662, 18, AUDIO_PRIORITY_MED }    // Quick shoot another ball
};
const byte NUM_COM_BALL_SAVED_URGENT = sizeof(comTracksBallSavedUrgent) / sizeof(comTracksBallSavedUrgent[0]);

// MULTIBALL COM tracks (671-675)
const AudioComTrackDef comTracksMultiball[] = {
  { 671, 34, AUDIO_PRIORITY_HIGH },  // First ball locked...
  { 672, 36, AUDIO_PRIORITY_HIGH },  // Second ball locked...
  { 673, 14, AUDIO_PRIORITY_HIGH },  // Multiball
  { 674, 33, AUDIO_PRIORITY_HIGH },  // Multiball; all rides are running
  { 675, 34, AUDIO_PRIORITY_HIGH }   // Every bumper is worth double...
};
const byte NUM_COM_MULTIBALL = sizeof(comTracksMultiball) / sizeof(comTracksMultiball[0]);

// COMPLIMENT COM tracks (701-714)
const AudioComTrackDef comTracksCompliment[] = {
  { 701,  9, AUDIO_PRIORITY_LOW },   // Good shot
  { 702, 20, AUDIO_PRIORITY_LOW },   // Awesome great work
  { 703, 10, AUDIO_PRIORITY_LOW },   // Great job
  { 704, 13, AUDIO_PRIORITY_LOW },   // Great shot
  { 705, 10, AUDIO_PRIORITY_LOW },   // Youve done it
  { 706, 15, AUDIO_PRIORITY_LOW },   // Mission accomplished
  { 707,  9, AUDIO_PRIORITY_LOW },   // Youve done it
  { 708, 13, AUDIO_PRIORITY_LOW },   // You did it
  { 709, 11, AUDIO_PRIORITY_LOW },   // Amazing
  { 710, 12, AUDIO_PRIORITY_LOW },   // Great job
  { 711, 27, AUDIO_PRIORITY_LOW },   // Great shot nicely done
  { 712, 11, AUDIO_PRIORITY_LOW },   // Well done
  { 713, 11, AUDIO_PRIORITY_LOW },   // Great work
  { 714, 20, AUDIO_PRIORITY_LOW }    // You did it great shot
};
const byte NUM_COM_COMPLIMENT = sizeof(comTracksCompliment) / sizeof(comTracksCompliment[0]);

// DRAIN COM tracks (721-748)
const AudioComTrackDef comTracksDrain[] = {
  { 721, 23, AUDIO_PRIORITY_LOW },   // Did you forget where the flipper buttons are
  { 722, 19, AUDIO_PRIORITY_LOW },   // Gravity called and you answered
  { 723, 19, AUDIO_PRIORITY_LOW },   // Have you been to an eye doctor lately
  { 724, 20, AUDIO_PRIORITY_LOW },   // Ill Pretend I Didnt See That
  { 725, 11, AUDIO_PRIORITY_LOW },   // Gravity wins
  { 726, 19, AUDIO_PRIORITY_LOW },   // Im So Sorry For Your Loss
  { 727,  9, AUDIO_PRIORITY_LOW },   // That was quick
  { 728, 36, AUDIO_PRIORITY_LOW },   // Maybe try playing with your eyes open...
  { 729, 29, AUDIO_PRIORITY_LOW },   // Nice drain, was that part of your strategy
  { 730, 20, AUDIO_PRIORITY_LOW },   // Oh I Didnt See That Coming
  { 731, 14, AUDIO_PRIORITY_LOW },   // Oh Thats Humiliating
  { 732, 11, AUDIO_PRIORITY_LOW },   // So long, ball
  { 733, 12, AUDIO_PRIORITY_LOW },   // Thats gotta hurt
  { 734, 23, AUDIO_PRIORITY_LOW },   // Thats The Saddest Thing Ive Ever Seen
  { 735,  7, AUDIO_PRIORITY_LOW },   // Yikes
  { 736, 17, AUDIO_PRIORITY_LOW },   // Oh that hurt to watch
  { 737, 17, AUDIO_PRIORITY_LOW },   // That Was Just Terrible
  { 738, 39, AUDIO_PRIORITY_LOW },   // Your ball saw the drain...
  { 739, 11, AUDIO_PRIORITY_LOW },   // Whoopsie daisy
  { 740, 11, AUDIO_PRIORITY_LOW },   // Get outta there, kid
  { 741, 13, AUDIO_PRIORITY_LOW },   // Hey, wrong way
  { 742, 14, AUDIO_PRIORITY_LOW },   // Kid not that way
  { 743, 12, AUDIO_PRIORITY_LOW },   // Leaving so soon
  { 744, 12, AUDIO_PRIORITY_LOW },   // No no no
  { 745, 15, AUDIO_PRIORITY_LOW },   // Not that way
  { 746, 11, AUDIO_PRIORITY_LOW },   // Not the exit
  { 747,  8, AUDIO_PRIORITY_LOW },   // Oh boy
  { 748, 11, AUDIO_PRIORITY_LOW }    // Where ya goin, kid
};
const byte NUM_COM_DRAIN = sizeof(comTracksDrain) / sizeof(comTracksDrain[0]);

// AWARD COM tracks (811-842)
const AudioComTrackDef comTracksAward[] = {
  { 811, 15, AUDIO_PRIORITY_HIGH },  // Special is lit
  { 812, 19, AUDIO_PRIORITY_HIGH },  // Reeee-plaaay
  { 821, 32, AUDIO_PRIORITY_HIGH },  // You lit three in a row, replay
  { 822, 44, AUDIO_PRIORITY_HIGH },  // You lit all four corners, five replays
  { 823, 46, AUDIO_PRIORITY_HIGH },  // You lit one, two and three, twenty replays
  { 824, 40, AUDIO_PRIORITY_HIGH },  // Five balls in the Gobble Hole - replay
  { 831, 18, AUDIO_PRIORITY_HIGH },  // You spelled SCREAMO
  { 841, 19, AUDIO_PRIORITY_HIGH },  // EXTRA BALL
  { 842, 20, AUDIO_PRIORITY_HIGH }   // Heres another ball; send it
};
const byte NUM_COM_AWARD = sizeof(comTracksAward) / sizeof(comTracksAward[0]);

// MODE COM tracks (1002-1003, 1005, 1101, 1111, 1201, 1211, 1301, 1311)
const AudioComTrackDef comTracksMode[] = {
  { 1002,  8, AUDIO_PRIORITY_HIGH },  // Jackpot
  { 1003, 13, AUDIO_PRIORITY_HIGH },  // Ten seconds left
  { 1005,  8, AUDIO_PRIORITY_HIGH },  // Time
  { 1101, 93, AUDIO_PRIORITY_HIGH },  // Lets ride the Bumper Cars
  { 1111, 18, AUDIO_PRIORITY_MED },   // Keep smashing the bumpers
  { 1201, 82, AUDIO_PRIORITY_HIGH },  // Lets play Roll-A-Ball
  { 1211, 18, AUDIO_PRIORITY_MED },   // Keep rolling over the hats
  { 1301, 71, AUDIO_PRIORITY_HIGH },  // Lets visit the Shooting Gallery
  { 1311, 19, AUDIO_PRIORITY_MED }    // Keep shooting at the Gobble Hole
};
const byte NUM_COM_MODE = sizeof(comTracksMode) / sizeof(comTracksMode[0]);

// *** SFX TRACK ARRAY ***
const AudioSfxTrackDef sfxTracks[] = {
  // DIAG SFX
  { 103, AUDIO_FLAG_NONE },   // Critical error
  { 104, AUDIO_FLAG_NONE },   // Switch Close 1000hz
  { 105, AUDIO_FLAG_NONE },   // Switch Open 500hz
  // TILT SFX
  { 211, AUDIO_FLAG_NONE },   // Buzzer
  // START SFX
  { 311, AUDIO_FLAG_NONE },   // Car honk aoooga
  { 401, AUDIO_FLAG_NONE },   // Scream player 1
  { 403, AUDIO_FLAG_LOOP },   // Lift, loopable (120s)
  { 404, AUDIO_FLAG_NONE },   // First drop multi-screams
  // MODE COMMON SFX
  { 1001, AUDIO_FLAG_NONE },  // School Bell stinger start
  { 1004, AUDIO_FLAG_NONE },  // Timer countdown
  { 1006, AUDIO_FLAG_NONE },  // Factory whistle stinger end
  // MODE BUMPER CAR HIT SFX (1121-1133)
  { 1121, AUDIO_FLAG_NONE },  // Car honk
  { 1122, AUDIO_FLAG_NONE },  // Car honk la cucaracha
  { 1123, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1124, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1125, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1126, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1127, AUDIO_FLAG_NONE },  // Car honk 2x
  { 1128, AUDIO_FLAG_NONE },  // Car honk 2x rubber
  { 1129, AUDIO_FLAG_NONE },  // Car honk 2x rubber
  { 1130, AUDIO_FLAG_NONE },  // Car honk aoooga
  { 1131, AUDIO_FLAG_NONE },  // Car honk diesel train
  { 1132, AUDIO_FLAG_NONE },  // Car honk
  { 1133, AUDIO_FLAG_NONE },  // Car honk truck air horn
  // MODE BUMPER CAR MISS SFX (1141-1148)
  { 1141, AUDIO_FLAG_NONE },  // Car crash
  { 1142, AUDIO_FLAG_NONE },  // Car crash
  { 1143, AUDIO_FLAG_NONE },  // Car crash
  { 1144, AUDIO_FLAG_NONE },  // Car crash
  { 1145, AUDIO_FLAG_NONE },  // Car crash x
  { 1146, AUDIO_FLAG_NONE },  // Cat screech
  { 1147, AUDIO_FLAG_NONE },  // Car crash x
  { 1148, AUDIO_FLAG_NONE },  // Car crash x
  // MODE BUMPER CAR ACHIEVED SFX
  { 1197, AUDIO_FLAG_NONE },  // Bell ding ding ding
  // MODE ROLL-A-BALL HIT SFX (1221-1225)
  { 1221, AUDIO_FLAG_NONE },  // Bowling strike
  { 1222, AUDIO_FLAG_NONE },  // Bowling strike
  { 1223, AUDIO_FLAG_NONE },  // Bowling strike
  { 1224, AUDIO_FLAG_NONE },  // Bowling strike
  { 1225, AUDIO_FLAG_NONE },  // Bowling strike
  // MODE ROLL-A-BALL MISS SFX (1241-1254)
  { 1241, AUDIO_FLAG_NONE },  // Glass
  { 1242, AUDIO_FLAG_NONE },  // Glass
  { 1243, AUDIO_FLAG_NONE },  // Glass
  { 1244, AUDIO_FLAG_NONE },  // Glass
  { 1245, AUDIO_FLAG_NONE },  // Glass
  { 1246, AUDIO_FLAG_NONE },  // Glass
  { 1247, AUDIO_FLAG_NONE },  // Glass
  { 1248, AUDIO_FLAG_NONE },  // Glass
  { 1249, AUDIO_FLAG_NONE },  // Glass
  { 1250, AUDIO_FLAG_NONE },  // Glass
  { 1251, AUDIO_FLAG_NONE },  // Glass
  { 1252, AUDIO_FLAG_NONE },  // Glass
  { 1253, AUDIO_FLAG_NONE },  // Glass
  { 1254, AUDIO_FLAG_NONE },  // Goat sound
  // MODE ROLL-A-BALL ACHIEVED SFX
  { 1297, AUDIO_FLAG_NONE },  // Ta da
  // MODE GOBBLE HOLE HIT SFX
  { 1321, AUDIO_FLAG_NONE },  // Slide whistle down
  // MODE GOBBLE HOLE MISS SFX (1341-1348)
  { 1341, AUDIO_FLAG_NONE },  // Ricochet
  { 1342, AUDIO_FLAG_NONE },  // Ricochet
  { 1343, AUDIO_FLAG_NONE },  // Ricochet
  { 1344, AUDIO_FLAG_NONE },  // Ricochet
  { 1345, AUDIO_FLAG_NONE },  // Ricochet
  { 1346, AUDIO_FLAG_NONE },  // Ricochet
  { 1347, AUDIO_FLAG_NONE },  // Ricochet
  { 1348, AUDIO_FLAG_NONE },  // Ricochet
  // MODE GOBBLE HOLE ACHIEVED SFX
  { 1397, AUDIO_FLAG_NONE }   // Applause mixed
};
const byte NUM_SFX_TRACKS = sizeof(sfxTracks) / sizeof(sfxTracks[0]);

// *** MUSIC TRACK ARRAYS ***
// CIRCUS music (2001-2019)
const AudioMusTrackDef musTracksCircus[] = {
  { 2001, 147 },  // Barnum and Baileys Favorite (2m27s)
  { 2002, 156 },  // Rensaz Race March (2m36s)
  { 2003,  77 },  // Twelfth Street Rag (1m17s)
  { 2004, 195 },  // Chariot Race, Ben Hur March (3m15s)
  { 2005,  58 },  // Organ Grinders Serenade (0m58s)
  { 2006, 155 },  // Hands Across the Sea (2m35s)
  { 2007, 132 },  // Field Artillery March (2m12s)
  { 2008, 102 },  // You Flew Over (1m42s)
  { 2009,  65 },  // Unter dem Doppeladler (1m5s)
  { 2010, 100 },  // Ragtime Cowboy Joe (1m40s)
  { 2011, 183 },  // Billboard March (3m3s)
  { 2012, 110 },  // El capitan (1m50s)
  { 2013, 119 },  // Smiles (1m59s)
  { 2014, 151 },  // Spirit of St. Louis March (2m31s)
  { 2015, 115 },  // The Free Lance (1m55s)
  { 2016, 139 },  // The Roxy March (2m19s)
  { 2017, 119 },  // The Stars and Stripes Forever (1m59s)
  { 2018, 141 },  // The Washington Post (2m21s)
  { 2019, 154 }   // Bombasto (2m34s)
};
const byte NUM_MUS_CIRCUS = sizeof(musTracksCircus) / sizeof(musTracksCircus[0]);

// SURF music (2051-2068)
const AudioMusTrackDef musTracksSurf[] = {
  { 2051, 131 },  // Miserlou (2m11s)
  { 2052, 185 },  // Bumble Bee Stomp (3m5s)
  { 2053, 177 },  // Wipe Out (2m57s)
  { 2054, 103 },  // Banzai Washout (1m43s)
  { 2055, 120 },  // Hava Nagila (2m0s)
  { 2056, 130 },  // Sabre Dance (2m10s)
  { 2057, 139 },  // Malaguena (2m19s)
  { 2058,  98 },  // Wildfire (1m38s)
  { 2059, 113 },  // The Wedge (1m53s)
  { 2060, 108 },  // Exotic (1m48s)
  { 2061, 182 },  // The Victor (3m2s)
  { 2062, 113 },  // Mr. Eliminator (1m53s)
  { 2063,  90 },  // Night Rider (1m30s)
  { 2064,  97 },  // The Jester (1m37s)
  { 2065, 129 },  // Pressure (2m9s)
  { 2066, 110 },  // Shootin Beavers (1m50s)
  { 2067, 127 },  // Riders in the Sky (2m7s)
  { 2068, 114 }   // Bumble Bee Boogie (1m54s)
};
const byte NUM_MUS_SURF = sizeof(musTracksSurf) / sizeof(musTracksSurf[0]);

#endif // PINBALL_AUDIO_TRACKS_H
