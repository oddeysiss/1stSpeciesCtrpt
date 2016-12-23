/*
 * Author: Mitchell Maciorski
 *
 * This program uses a backtracking algorithm to write a
 * first species counterpoint as a .csd file to be played
 * using CSound. Upon running this program, the user is
 * prompted to provide the key, the number of measures, and
 * the tempo of the piece. Given these fields, the output
 * will be a random composition fitting all constraints.
*/

#include "stdafx.h"
#include <fstream> // ofstream, ifstream
#include <iostream> // cout
#include <string> 
#include <sstream>
#include <map>
#include <vector>
#include <stdlib.h> // srand, rand
#include <time.h>
#include <stdio.h> // NULL

using namespace std;

// Constants to define the range of each voice.
const double ALTO[] = { 196.00, 698.47 };
const double TENOR[] = { 130.81, 523.26 };

// API---------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// FILE WRITING -----------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

ofstream startFile(string filename);
// Opens and begins writing to file. Returns ofstream for further writing.

void writeMelody(ofstream& myfile);
// Writes the cantus and ctrpt melodies to the file.

vector<int> writeCantusMelody(ofstream& myfile, vector<string> musicKey, int octIndicator);
// Generates and writes cantus melody to file. Returns cantus notes for further use.

void writeCtrptMelody(ofstream& myfile, vector<string> musicKey,
    vector<int> cantusNotes, int octIndicator);
// Generates and writes ctrpt melody to file.

void endFile(ofstream& myfile);

//-------------------------------------------------------------------------------------------------
// COMPOSITION ------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

vector<int> fillCtrptMelody(vector<string> musicKey, vector<int> cantusNotes, int octIndicator);
// Container function for backtrackFillCtrptMelody(). Returns finished ctrpt melody.

vector<int> backtrackFillCtrptMelody(vector<int>& ctrptNotes, map<int, float> notes,
    vector<int> cantusNotes);
// Generates / returns ctrpt melody.

vector<string> getMusicKey();
// Reads in list of keys and returns a list of note names corresponding to the key given by user
// input.

map<int, float> getNotes(vector<string> musicKey, int octIndicator, const double range[]);
// Reads in NoteFrequencies.txt and returns a map from two digit int (note position in specified
// key & note octave) to float (corresponding frequencies).

int notePos(string note, vector<string> musicKey, int octIndicator);
// Converts a string (C1, A#2, etc...) to a 2-digit int specifying octave (10's place) and
// position in musical key (1 - 7 in 1's place). OctIndicator orients the process such that
// the octave digit increments with the tonic note.

int getInterval(int note1, int note2);
// Returns the interval between two notes as an integer.

int reduceInterval(int interval);
// Reduces an interval to its smallest form... (e.g. 10 -> 3)

bool isConsonant(int interval);
// Returns true if provided interval is consonant, false otherwise.

int getOctaveIndicator(vector<string> musicKey);
// Returns the note in the key (I, II, ...) that the octave increments at by comparing notes
// to "C". Returns -1 upon error.

//-------------------------------------------------------------------------------------------------
// CONSTRAINT SATTISFACTION -----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

map<int, float> getAllowedCantusNotes(map<int, float> notes, int prevNotes[],
    int noteNum, int numNotes);
// Checks all available notes against cantus constraints and returns all valid cantus notes as a
// map from int (note position / octave) to float (note frequency).

map<int, float> getAllowedCtrptNotes(vector<int> ctrptNotes, vector<int> cantusNotes,
    map<int, float> notes);
// Checks all available notes against ctrpt constraints and returns all valid ctrpt notes as a map
// from int(note position / octave) to float (note frequency).

void removeParallelFifths(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes,
    vector<int> cantusNotes);
// Imposes constraint on ctrptNotes.

void removeParallelEighths(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes,
    vector<int> cantusNotes);
// Imposes constraint on ctrptNotes.

void remove3xLeap(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes);
// Imposes constraint on ctrptNotes.

void removeOppositeLeaps(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes);
// Imposes constraint on ctrptNotes.

void remove4xIntervalOrNote(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes,
    vector<int> cantusNotes);
// Imposes constraint on ctrptNotes.

//-------------------------------------------------------------------------------------------------
// UTILS ------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

vector<int> getKeyList(map<int, float> notes);
// Returns a list of keys from a given map.

void seedRand();

int randomNoteKey(vector<int> noteKeys);

int getTempo();
// Get the tempo in BPM.

int getNumMeasures();
// Get the number of measures to be written.

int calcTotalNotes(int numMeasures);
// Calculate the total amount of time in seconds of the melody.



// END API-----------------------------------------------------------------------------------------



/**************************************************************************************************
*                                      FILE WRITING                                               *
**************************************************************************************************/

// Opens and writes the first part of the score. Returns ofstream for further writing.
ofstream startFile(string filename){
    ofstream myfile(filename);
    if (myfile.is_open()) {
        myfile << "<CsoundSynthesizer>\n";
        myfile << "<CsOptions>\n";
        myfile << "-odac\n";
        myfile << "</CsOptions>\n";
        myfile << "<CsInstruments>\n";
        myfile << "instr 1\n";
        myfile << "aSin vco2 0dbfs/4, p4\n";
        myfile << "out aSin\n";
        myfile << "endin\n\n";
        myfile << "instr 2\n";
        myfile << "aSin vco2 0dbfs/4, p4\n";
        myfile << "out aSin\n";
        myfile << "endin\n\n";
        myfile << "</CsInstruments>\n";
        myfile << "<CsScore>\n";
        return myfile;
    }
    else {
        cout << "Unable to open file.";
        return ofstream("/dev/null");
    }
}

// Writes the cantus and ctrpt melodies to the file.
void writeMelody(ofstream& myfile) {
    if (myfile.is_open()) {
        vector<string> musicKey;
        do {
            musicKey = getMusicKey();
        } while (musicKey.size() == 0);
        int octIndicator = getOctaveIndicator(musicKey);

        vector<int> cantusNotes = writeCantusMelody(myfile, musicKey, octIndicator);
        writeCtrptMelody(myfile, musicKey, cantusNotes, octIndicator);
        myfile << "</CsScore>\n";
        myfile << "</CsoundSynthesizer>";
    }
}

// Generates and writes cantus melody to file. Returns cantus notes for further use.
vector<int> writeCantusMelody(ofstream& myfile, vector<string> musicKey, int octIndicator) {
    vector<int> cantusNotes;
    if (myfile.is_open()) {
        map<int, float> notes = getNotes(musicKey, octIndicator, ALTO);
        vector<int> noteKeys = getKeyList(notes);
        
        int tempo = getTempo();
        int numMeasures = getNumMeasures();
        int totalNotes = calcTotalNotes(numMeasures);
        int noteKey;
        float note;
        int noteNum = 1;
        int prevNotes[] = { -1, -1 };

        myfile << "t 0 " << tempo << endl << endl;

        while (noteNum <= totalNotes) {
            map<int, float> allowedNotes = getAllowedCantusNotes(notes, 
                prevNotes, noteNum, totalNotes);
            vector<int> allowedKeys = getKeyList(allowedNotes);
            noteKey = randomNoteKey(allowedKeys);
            cantusNotes.push_back(noteKey);
            note = allowedNotes[noteKey];
            prevNotes[1] = prevNotes[0];
            prevNotes[0] = noteKey;
            myfile << "i1 " << noteNum - 1 << " 1 " << note << endl;
            noteNum += 1;
        }
    }
    return cantusNotes;
}

// Generates and writes ctrpt melody to file.
void writeCtrptMelody(ofstream& myfile, vector<string> musicKey,
    vector<int> cantusNotes, int octIndicator) {
    if (myfile.is_open()) {
        map<int, float> notes = getNotes(musicKey, octIndicator, TENOR);
        vector<int> noteKeys = getKeyList(notes);
        vector<int> ctrptMelody = fillCtrptMelody(musicKey, cantusNotes, octIndicator);
        for (unsigned i = 0; i < ctrptMelody.size(); i++) {
            float note = notes[ctrptMelody[i]];
            myfile << "i2 " << i << " 1 " << note << endl;
        }
    }
}

// Closes the score.
void endFile(ofstream& myfile) {
    myfile.close();
}

/**************************************************************************************************
*                                        COMPOSITION                                              *
**************************************************************************************************/

// Container function for backtrackFillCtrptMelody(). Returns finished ctrpt melody.
vector<int> fillCtrptMelody(vector<string> musicKey, vector<int> cantusNotes, int octIndicator) {
    vector<int> ctrptNotes;
    map<int, float> notes = getNotes(musicKey, octIndicator, TENOR);
    return backtrackFillCtrptMelody(ctrptNotes, notes, cantusNotes);
}

// Generates / returns ctrpt melody.
vector<int> backtrackFillCtrptMelody(vector<int>& ctrptNotes, map<int, float> notes,
    vector<int> cantusNotes) {
    // Base case - finished writing ctrpt melody
    if (ctrptNotes.size() == cantusNotes.size()) {
        return ctrptNotes;
    }

    map<int, float> allowedNotes = getAllowedCtrptNotes(ctrptNotes, cantusNotes, notes);
    // Failure
    if (allowedNotes.size() == 0) {
        ctrptNotes.push_back(-1);
        return ctrptNotes;
    }

    // Keep track of visited allowed notes
    map<int, float> visited;

    vector<int> allowedNotesKeys;
    for (auto it : allowedNotes) {
        allowedNotesKeys.push_back(it.first);
    }

    // Try to find a solution for a random allowed note
    while (visited.size() < allowedNotes.size()) {
        int noteKey;
        do {
            noteKey = randomNoteKey(allowedNotesKeys);
        } while (visited.count(noteKey) != 0);
        visited[noteKey] = allowedNotes[noteKey];
        ctrptNotes.push_back(noteKey);
        vector<int> buffer = backtrackFillCtrptMelody(ctrptNotes, notes, cantusNotes);
        // If the assignment was successful, return the complete melody.
        if (buffer.back() != -1) {
            return buffer;
        }
        // For failure, get rid of the indicator and last note.
        ctrptNotes.pop_back();
        ctrptNotes.pop_back();
    }

    // No assignment successful - failure.
    ctrptNotes.push_back(-1);
    return ctrptNotes;
}

// Reads in list of keys and returns a list of note names corresponding to the key given by user
// input.
vector<string> getMusicKey() {
    string inputKey;
    cout << "Please input desired key (A, B, C#, etc...): ";
    cin >> inputKey;

    ifstream keyTable("Keys.txt");
    if (keyTable.is_open()) {
        // initialize a match flag, a line holder, and the list to hold notes in the key.
        bool match = false;
        string line;
        vector<string> myKey;

        // Initialize the holders for the stringstream.
        string I, II, III, IV, V, VI, VII;

        // Eat the header
        getline(keyTable, line);

        // Try and fill the list.
        while (getline(keyTable, line) && !match) {
            stringstream ss;
            ss << line;
            ss >> I;
            if (I == inputKey) {
                ss >> II >> III >> IV >> V >> VI >> VII;
                myKey.push_back(I);
                myKey.push_back(II);
                myKey.push_back(III);
                myKey.push_back(IV);
                myKey.push_back(V);
                myKey.push_back(VI);
                myKey.push_back(VII);
                match = true;
            }
        }
        return myKey;
    }
    else {
        cout << "Unable to read keys file.\n";
        vector<string> noKey;
        return noKey;
    }
}

// Reads in NoteFrequencies.txt and populates a map from two digit int (note position in specified
// key & note octave) to float (corresponding frequencies).
map<int, float> getNotes(vector<string> musicKey, int octIndicator, const double range[]) {

    ifstream noteTable("NoteFrequencies.txt");
    map<int, float> notes;
    if (noteTable.is_open()) {
        // line holds lines... note will help make a key in a map to corresp. freq.
        string line;
        string note;
        float freq;

        // Eat the header...
        getline(noteTable, line);

        while (getline(noteTable, line)) {
            // ss should take the line and return the items in the first two columns.
            stringstream ss;
            ss << line;
            ss >> note >> freq;

            // We only want to add the note if it's in the specified range.
            if (freq >= range[0] && freq <= range[1]) {
                // We also will only add the note if it's in our desired key.
                bool match = false;
                for (auto it : musicKey) {
                    // string.find(substr) returns string: :npos if substr isn't in string.
                    if (note.find(it) != string::npos) {
                        // Making sure both notes are sharp or both are natural.
                        if (note.find("#") == it.find("#")) {
                            match = true;
                        }
                    }
                }
                if (match) {
                    int key = notePos(note, musicKey, octIndicator);
                    if (key != -1) {
                        notes[key] = freq;
                    }
                }
            }
        }
        noteTable.close();
        return notes;
    }
    cout << "no dice" << endl;
    map<int, float> empty;
    return empty;
}

// Converts a string (C1, A#2, etc...) to a 2-digit int specifying octave (10's place) and
// position in musical key (1 - 7 in 1's place). OctIndicator orients the process such that
// the octave digit increments with the correct note.
int notePos(string note, vector<string> musicKey, int octIndicator) {
    int notepos = -1;
    int octave = note.back() - '0';
    note.pop_back();
    for (unsigned i = 0; i < musicKey.size(); i++) {
        if (note.compare(musicKey[i]) == 0) {
            // Plus one to adhere to music convention... tonic is 1, not 0.
            notepos = i + 1;
            break;
        }
    }
    if (notepos == -1) {
        cout << "Error in notePos(): note not in musicKey.\n";
        return notepos;
    }
    if (notepos >= octIndicator) {
        return (10 * octave) + notepos;
    }
    else {
        return (10 * (octave + 1)) + notepos;
    }
}

// Returns the interval between two notes as an integer. 
int getInterval(int note1, int note2) {
    // /10 & %10 because notes 1 & 2 are exclusively for two digit numbers.
    // Note that the + 1 posInt is to adhere to music notation that the interval
    // from a note to itself (C0 to C0 for example) is one.The sevens account
    // for multi - octave intervals.
    int first = 7 * (note1 / 10) + (note1 % 10);
    int second = 7 * (note2 / 10) + (note2 % 10);
    return abs(first - second) + 1;
}

// Reduces an interval to its smallest form... (e.g. 10 -> 3)
int reduceInterval(int interval) {
    if (interval <= 7) {
        return interval;
    }
    return reduceInterval(interval - 7);
}

// Returns true if provided interval is consonant, false otherwise.
bool isConsonant(int interval) {
    int redInt = reduceInterval(interval);
    if (redInt == 1 || redInt == 3 || redInt == 5 || redInt == 6) {
        return true;
    }
    return false;
}

// Returns the note in the key (I, II, ...) that the octave increments at by comparing notes
// to "C". Returns -1 upon error.
int getOctaveIndicator(vector<string> musicKey) {
    string tonic = musicKey[0];
    int proximityIndicator = tonic.compare("C");
    if (proximityIndicator == 0) {
        return 1;
    }
    if (proximityIndicator < 0) {
        for (unsigned i = 1; i < musicKey.size(); i++) {
            if (musicKey[i].compare("C") >= 0) {
                // + 1 to fit in with tonic = 1, not 0.
                return i + 1;
            }
        }
        return -1;
    }
    else {
        for (int i = musicKey.size() - 1; i > 0; i--) {
            if (musicKey[i].compare("C") < 0) {
                // + 2 for tonic = 1 and we've found the note one before
                // the switch.
                return i + 2;
            }
            else if (musicKey[i].compare("C") == 0) {
                return i + 1;
            }
        }
        return -1;
    }
}

/**************************************************************************************************
*                                  CONSTRAINT SATISFACTION                                        *
**************************************************************************************************/

// Checks all available notes against cantus constraints and returns all valid cantus notes as a
// map from int (note position / octave) to float (note frequency).
map<int, float> getAllowedCantusNotes(map<int, float> notes, int prevNotes[],
    int noteNum, int totalNotes) {
    map<int, float> allowedNotes;
    // Start with the tonic.
    if (noteNum == 1) {
        for (auto it : notes) {
            if (it.first % 10 == 1) {
                allowedNotes[it.first] = it.second;
            }
        }
    }
    // End with the tonic, but it should be stepwise from the last note... so we'll get
    // to the last note needing to be II or VII. 
    else if (noteNum == totalNotes) {
        for (auto it : notes) {
            int interval = getInterval(prevNotes[0], it.first);
            if ((interval == 2) && (it.first % 10 == 1)) {
                allowedNotes[it.first] = it.second;
            }
        }
    }
    // Second to last note must be II or VII, and i don't want leaps larger than a sixth.
    else if (noteNum == totalNotes - 1) {
        for (auto it : notes) {
            int interval = getInterval(prevNotes[0], it.first);
            if ((interval <= 6) && ((it.first % 10 == 2) || (it.first % 10 == 7))) {
                allowedNotes[it.first] = it.second;
            }
        }
    }
    // Second note will have prev[1] = -1.
    else if (prevNotes[1] == -1) {
        for (auto it : notes) {
            int interval = getInterval(prevNotes[0], it.first);
            // Just don't leap too far.
            if (interval <= 6) {
                allowedNotes[it.first] = it.second;
            }
        }
    }
    else {
        int prevInterval = getInterval(prevNotes[0], prevNotes[1]);
        // Don't repeat notes more than twice.
        if (prevInterval == 1) {
            for (auto it : notes) {
                int interval = getInterval(prevNotes[0], it.first);
                if (interval <= 6 && interval != 1) {
                    allowedNotes[it.first] = it.second;
                }
            }
        }
        // Follow a small leap with stepwise motion. 
        else if (prevInterval == 3) {
            for (auto it : notes) {
                int interval = getInterval(prevNotes[0], it.first);
                if (interval <= 2) {
                    allowedNotes[it.first] = it.second;
                }
            }
        }
        // Large leaps are followed by contrary stepwise motion.
        else if (prevInterval >= 4) {
            int direction = prevNotes[0] - prevNotes[1];
            for (auto it : notes) {
                int interval = getInterval(prevNotes[0], it.first);
                int intDir = it.first - prevNotes[0];
                if (direction > 0) {
                    if (interval == 2 && intDir < 0) {
                        allowedNotes[it.first] = it.second;
                    }
                }
                if (direction < 0) {
                    if (interval == 2 && intDir > 0) {
                        allowedNotes[it.first] = it.second;
                    }
                }
            }
        }
        // After stepwise motion do whatever you want.
        else {
            for (auto it : notes) {
                int interval = getInterval(prevNotes[0], it.first);
                if (interval <= 6) {
                    allowedNotes[it.first] = it.second;
                }
            }
        }
    }
    return allowedNotes;
}

// Checks all available notes against ctrpt constraints and returns all valid ctrpt notes as a map
// from int(note position / octave) to float (note frequency).
map<int, float> getAllowedCtrptNotes(vector<int> ctrptNotes, vector<int> cantusNotes,
    map<int, float> notes) {
    // Initialize map of allowed notes.
    map<int, float> allowedCtrptNotes;

    // First note must be tonic.
    if (ctrptNotes.size() == 0) {
        for (auto it : notes) {
            if (it.first % 10 == 1) {
                allowedCtrptNotes[it.first] = it.second;
            }
        }
        return allowedCtrptNotes;
    }

    // Penultimate note must be either II or VII, depending on cantus.
    if (ctrptNotes.size() == cantusNotes.size() - 2) {
        // Cantus note is ii...
        if (cantusNotes.end()[-2] % 10 == 2) {
            // So vii is allowed.
            for (auto it : notes) {
                if (it.first % 10 == 7) {
                    allowedCtrptNotes[it.first] = it.second;
                }
            }
        } // Cantus note is vii ...
        else {
            // So ii is allowed.
            for (auto it : notes) {
                if (it.first % 10 == 2) {
                    allowedCtrptNotes[it.first] = it.second;
                }
            }
        }
        return allowedCtrptNotes;
    }

    // Last note must be tonic.
    if (ctrptNotes.size() == cantusNotes.size() - 1) {
        for (auto it : notes) {
            int interval = getInterval(ctrptNotes.back(), it.first);
            if (it.first % 10 == 1 && interval == 2) {
                allowedCtrptNotes[it.first] = it.second;
            }
        }
        return allowedCtrptNotes;
    }

    // Need to compare ctrpt notes to cantus notes to determine which notes are allowed.
    // Add every note that forms a consonance with the cantus and then prune the list based
    // on previous ctrpt notes and previous intervals formed between the two melodies.
    int currIndex = ctrptNotes.size() - 1;
    // Getting all notes that could possibly be allowed.
    for (auto it : notes) {
        int cantusInterval = getInterval(cantusNotes[currIndex], it.first);
        int ctrptInterval = getInterval(ctrptNotes.back(), it.first);
        if (isConsonant(cantusInterval) && cantusInterval <= 12 && 
                it.first < cantusNotes[currIndex] && ctrptInterval <= 6) {
                allowedCtrptNotes[it.first] = it.second;
            }
    }

    if (ctrptNotes.size() == 1) {
        removeParallelFifths(allowedCtrptNotes, ctrptNotes, cantusNotes);
        removeParallelEighths(allowedCtrptNotes, ctrptNotes, cantusNotes);
    }
    else if (ctrptNotes.size() == 2) {
        removeParallelFifths(allowedCtrptNotes, ctrptNotes, cantusNotes);
        removeParallelEighths(allowedCtrptNotes, ctrptNotes, cantusNotes);
        removeOppositeLeaps(allowedCtrptNotes, ctrptNotes);
    }
    else{
        removeParallelFifths(allowedCtrptNotes, ctrptNotes, cantusNotes);
        removeParallelEighths(allowedCtrptNotes, ctrptNotes, cantusNotes);
        removeOppositeLeaps(allowedCtrptNotes, ctrptNotes);
        remove3xLeap(allowedCtrptNotes, ctrptNotes);
        remove4xIntervalOrNote(allowedCtrptNotes, ctrptNotes, cantusNotes);
    }
    return allowedCtrptNotes;
}

// Imposes constraint on ctrptNotes.
void removeParallelFifths(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes, vector<int> cantusNotes) {
    if (allowedCtrptNotes.size() == 0) {
        return;
    }

    int currIndex = ctrptNotes.size() - 1;
    int prevInterval = getInterval(ctrptNotes.back(), cantusNotes[currIndex]);
    prevInterval = reduceInterval(prevInterval);

    if (prevInterval == 5) {
        vector<int> removalList;
        for (auto it : allowedCtrptNotes) {
            int interval = getInterval(it.first, cantusNotes[currIndex]);
            if (reduceInterval(interval) == 5) {
                removalList.push_back(it.first);
            }
        }
        for (auto it : removalList) {
            allowedCtrptNotes.erase(it);
        }
    }
}

// Imposes constraint on ctrptNotes.
void removeParallelEighths(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes, vector<int> cantusNotes) {
    if (allowedCtrptNotes.size() == 0) {
        return;
    }

    int currIndex = ctrptNotes.size() -1;
    int prevInterval = getInterval(ctrptNotes.back(), cantusNotes[currIndex]);
    prevInterval = reduceInterval(prevInterval);

    if (prevInterval == 1) {
        vector<int> removalList;
        for (auto it : allowedCtrptNotes) {
            int interval = getInterval(it.first, cantusNotes[currIndex]);
            if (reduceInterval(interval) == 1) {
                removalList.push_back(it.first);
            }
        }
        for (auto it : removalList) {
            allowedCtrptNotes.erase(it);
        }
    }
}

// Imposes constraint on ctrptNotes.
void remove3xLeap(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes) {
    if (allowedCtrptNotes.size() == 0) {
        return;
    }

    int interval1 = getInterval(ctrptNotes.back(), ctrptNotes.end()[-2]);
    int interval2 = getInterval(ctrptNotes.end()[-2], ctrptNotes.end()[-3]);
    if (interval1 >= 3 && interval2 >= 3) {
        vector<int> removalList;
        for (auto it: allowedCtrptNotes) {
            int interval = getInterval(it.first, ctrptNotes.back());
            if (interval >= 3) {
                removalList.push_back(it.first);
            }
        }
        for (auto it : removalList) {
            allowedCtrptNotes.erase(it);
        }
    }
}

// Imposes constraint on ctrptNotes.
void removeOppositeLeaps(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes) {
    if (allowedCtrptNotes.size() == 0) {
        return;
    }

    int prevInterval = getInterval(ctrptNotes.end()[-1], ctrptNotes.end()[-2]);

    if (prevInterval >= 3) {
        bool prevDir = (ctrptNotes.end()[-1] - ctrptNotes.end()[-2]) > 0;
        vector<int> removalList;
        for (auto it : allowedCtrptNotes) {
            int interval = getInterval(it.first, ctrptNotes.back());
            bool dir = (it.first - ctrptNotes.back()) > 0;
            if (interval >= 3 && dir != prevDir) {
                removalList.push_back(it.first);
            }
        }
        for (auto it : removalList) {
            allowedCtrptNotes.erase(it);
        }
    }
}

// Imposes constraint on ctrptNotes.
void remove4xIntervalOrNote(map<int, float>& allowedCtrptNotes, vector<int> ctrptNotes,
    vector<int> cantusNotes) {
    if (allowedCtrptNotes.size() == 0) {
        return;
    }

    // Same note three times.
    if (ctrptNotes.end()[-1] == ctrptNotes.end()[-2] &&
        ctrptNotes.end()[-1] == ctrptNotes.end()[-3]) {
        allowedCtrptNotes.erase(ctrptNotes.back());
    }

    int prevIntervals[] = {0, 0, 0};
    int currIndex = ctrptNotes.size() - 1;
    prevIntervals[0] = getInterval(ctrptNotes.back(), cantusNotes[currIndex]);
    prevIntervals[1] = getInterval(ctrptNotes.end()[-2], cantusNotes[currIndex - 1]);
    prevIntervals[2] = getInterval(ctrptNotes.end()[-3], cantusNotes[currIndex - 2]);

    // Same interval 3 times.
    if (prevIntervals[0] == prevIntervals[1] && prevIntervals[0] == prevIntervals[2]) {
        vector<int> removalList;
        for (auto it : allowedCtrptNotes) {
            int interval = getInterval(it.first, cantusNotes[currIndex]);
            if (interval == prevIntervals[0]) {
                removalList.push_back(it.first);
            }
        }
        for (auto it : removalList) {
            allowedCtrptNotes.erase(it);
        }
    }
}

// Returns a list of keys from a given map.
vector<int> getKeyList(map<int, float> notes) {
    vector<int> keyList;
    for (auto it : notes) {
        keyList.push_back(it.first);
    }
    return keyList;
}

/**************************************************************************************************
*                                           UTILS                                                 *
**************************************************************************************************/

void seedRand() {
    srand(time(0));
}

int randomNoteKey(vector<int> noteKeys) {
    int randIndex = rand() % noteKeys.size();
    return noteKeys[randIndex];
}

// Get the tempo in BPM.
int getTempo() {
    int tempo;
    cout << "Please enter your desired tempo (BPM): ";
    cin >> tempo;
    return tempo;
}

// Get the number of measures to be written.
int getNumMeasures() {
    int numMeasures;
    cout << "Please enter the desired number of measures: ";
    cin >> numMeasures;
    return numMeasures;
}

// Calculate the total amount of time in seconds of the melody.
int calcTotalNotes(int numMeasures) {
    // 4 beats per measure * numMeasures
    return 4 * numMeasures;
}

int main()
{
    seedRand();
    ofstream myfile = startFile("counterpoint.csd");
    writeMelody(myfile);
    endFile(myfile);
    return 0;
}
