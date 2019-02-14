#include "ProgressState.hpp"

// ProgressState
ProgressState::ProgressState() {
    for (int i = 0; i < 8; i++) {
        chords[i].setVoltages(music::ChordTable[1].root, offset);
    }
}

void ProgressState::setMode(int m) {
    if (mode != m) {
        mode = m;
        if(chordMode) { 
            for (int i = 0; i < 8; i++) {
                music::getRootFromMode(mode, key, chords[i].modeDegree, &(chords[i].rootNote), &(chords[i].quality));
            }
        }
        dirty = true;
    }
}

void ProgressState::setKey(int k) {
    if (key != k) {
        key = k;
        if(chordMode) { 
            for (int i = 0; i < 8; i++) {
                music::getRootFromMode(mode, key, chords[i].modeDegree, &(chords[i].rootNote), &(chords[i].quality));
            }
        }
        dirty = true;
    }
}

json_t *ProgressState::toJson() {
    json_t *rootJ = json_object();

	// pChord
    json_t *rootNotesJ      = json_array();
    json_t *qualitysJ       = json_array();
    json_t *chordsJ         = json_array();
    json_t *modeDegreesJ    = json_array();
    json_t *inversionsJ     = json_array();
    json_t *gatesJ          = json_array();

    for (int i = 0; i < 8; i++) {
        json_t *rootNoteJ   = json_integer(chords[i].rootNote);
        json_t *qualityJ    = json_integer(chords[i].quality);
        json_t *chordJ      = json_integer(chords[i].chord);
        json_t *modeDegreeJ = json_integer(chords[i].modeDegree);
        json_t *inversionJ  = json_integer(chords[i].inversion);
        json_t *gateJ       = json_boolean(chords[i].gate);

        json_array_append_new(rootNotesJ,   rootNoteJ);
        json_array_append_new(qualitysJ,    qualityJ);
        json_array_append_new(chordsJ,      chordJ);
        json_array_append_new(modeDegreesJ, modeDegreeJ);
        json_array_append_new(inversionsJ,  inversionJ);
        json_array_append_new(gatesJ,       gateJ);
    }

    json_object_set_new(rootJ, "rootnote",      rootNotesJ);
    json_object_set_new(rootJ, "quality",       qualitysJ);
    json_object_set_new(rootJ, "chord",         chordsJ);
    json_object_set_new(rootJ, "modedegree",    modeDegreesJ);
    json_object_set_new(rootJ, "inversion",     inversionsJ);
    json_object_set_new(rootJ, "gate",          gatesJ);

    // offset
    json_t *offsetJ = json_integer((int) offset);
    json_object_set_new(rootJ, "offset", offsetJ);

    // chordMode
    json_t *chordModeJ = json_integer((int) chordMode);
    json_object_set_new(rootJ, "chordMode", chordModeJ);

    return rootJ;
}

void ProgressState::fromJson(json_t *rootJ) {

	// rootNote
    json_t *rootNotesJ = json_object_get(rootJ, "rootnote");
    if (rootNotesJ) {
        for (int i = 0; i < 8; i++) {
            json_t *rootNoteJ = json_array_get(rootNotesJ, i);
            if (rootNoteJ)
                chords[i].rootNote = json_integer_value(rootNoteJ);
        }
    }

	// quality
    json_t *qualitysJ = json_object_get(rootJ, "quality");
    if (qualitysJ) {
        for (int i = 0; i < 8; i++) {
            json_t *qualityJ = json_array_get(qualitysJ, i);
            if (qualityJ)
                chords[i].quality = json_integer_value(qualityJ);
        }
    }

	// chord
    json_t *chordsJ = json_object_get(rootJ, "chord");
    if (chordsJ) {
        for (int i = 0; i < 8; i++) {
            json_t *chordJ = json_array_get(chordsJ, i);
            if (chordJ)
                chords[i].chord = json_integer_value(chordJ);
        }
    }

	// modeDegree
    json_t *modeDegreesJ = json_object_get(rootJ, "modedegree");
    if (modeDegreesJ) {
        for (int i = 0; i < 8; i++) {
            json_t *modeDegreeJ = json_array_get(modeDegreesJ, i);
            if (modeDegreeJ)
                chords[i].modeDegree = json_integer_value(modeDegreeJ);
        }
    }

	// inversion
    json_t *inversionsJ = json_object_get(rootJ, "inversion");
    if (inversionsJ) {
        for (int i = 0; i < 8; i++) {
            json_t *inversionJ = json_array_get(inversionsJ, i);
            if (inversionJ)
                chords[i].rootNote = json_integer_value(inversionsJ);
        }
    }

	// gates
    json_t *gatesJ = json_object_get(rootJ, "gate");
    if (gatesJ) {
        for (int i = 0; i < 8; i++) {
            json_t *gateJ = json_array_get(gatesJ, i);
            if (gateJ)
                chords[i].gate = json_boolean_value(gateJ);
        }
    }

    // offset
    json_t *offsetJ = json_object_get(rootJ, "offset");
    if (offsetJ)
        offset = json_integer_value(offsetJ);

    // chordMode
    json_t *chordModeJ = json_object_get(rootJ, "chordMode");
    if (chordModeJ)
        chordMode = json_integer_value(chordModeJ);

}


// ProgressState

// Root/Degree menu
void RootItem::onAction(const event::Action &e) {
    pChord->rootNote = root;
    pChord->dirty = true;
}

void DegreeItem::onAction(const event::Action &e) {
    pChord->modeDegree = degree;
    pChord->dirty = true;

    // Root and chord can get updated
    // From the input key, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
    music::getRootFromMode(pState->mode, pState->key, pChord->modeDegree, &(pChord->rootNote), &(pChord->quality));

}

void RootChoice::onAction(const event::Action &e) {
    if (!pState)
        return;

    ui::Menu *menu = createMenu();
    if (pState->chordMode) {
        menu->addChild(createMenuLabel("Degree"));
        for (int i = 0; i < music::NUM_DEGREES; i++) {
            DegreeItem *item = new DegreeItem;
            item->pState = pState;
            item->pChord = &(pState->chords[pStep]);
            item->degree = i;
            item->text = music::degreeNames[i * 3];
            menu->addChild(item);
        }
    } else {
        menu->addChild(createMenuLabel("Root Note"));
        for (int i = 0; i < music::NUM_NOTES; i++) {
            RootItem *item = new RootItem;
            item->pChord = &(pState->chords[pStep]);
            item->root = i;
            item->text = music::noteNames[i];
            menu->addChild(item);
        }
    }
}

void RootChoice::step() {
    if (!pState) {
        text = "";
        return;
    }

    ProgressChord &pC = pState->chords[pStep];
    music::InversionDefinition &inv = pState->knownChords.chords[pC.chord].inversions[pC.inversion];
    
    if(pState->nSteps > pStep) {
        color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
    } else {
        color = nvgRGBA(0xFF, 0x00, 0x00, 0xFF);
    }

    text = std::to_string(pStep + 1) + ": ";

    if (pState->chordMode) {
        text += inv.getName(pState->mode, pState->key, pC.modeDegree, pC.rootNote);
        text += " " + music::degreeNames[pC.modeDegree * 3 + pC.quality];
    } else {
        text += inv.getName(pC.rootNote);
    }
}
// Root/Degree menu

// Chord menu
void ChordItem::onAction(const event::Action &e)  {
    pChord->chord = chord;
    pChord->dirty = true;
}

struct ChordSubsetMenu : MenuItem {
	ProgressState *pState;
	int pStep;
    int start;
    int end;
    Menu *createChildMenu() override {
        Menu *menu = new Menu;
        for (int i = start; i < end; i++) {
            ChordItem *item = new ChordItem;
            item->pChord = &(pState->chords[pStep]);
            item->chord = i;
            item->text = music::ChordTable[i].name;
            menu->addChild(item);
        }
        return menu;
    }
};

void ChordChoice::onAction(const event::Action &e) {
    if (!pState)
        return;

    ui::Menu *menu = createMenu();
    ChordSubsetMenu *majorItem = createMenuItem<ChordSubsetMenu>("Major");
    majorItem->pState = pState;
    majorItem->pStep = pStep;
    majorItem->start = 1;
    majorItem->end = 70;
    menu->addChild(majorItem);

    ChordSubsetMenu *minorItem = createMenuItem<ChordSubsetMenu>("Minor");
    minorItem->pState = pState;
    minorItem->pStep = pStep;
    minorItem->start = 71;
    minorItem->end = 90;
    menu->addChild(minorItem);

    ChordSubsetMenu *otherItem = createMenuItem<ChordSubsetMenu>("Other");
    otherItem->pState = pState;
    otherItem->pStep = pStep;
    otherItem->start = 91;
    otherItem->end = 99;
    menu->addChild(otherItem);

}

void ChordChoice::step() {
    if (!pState) {
        text = "";
        return;
    }

    if(pState->nSteps > pStep) {
        color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
    } else {
        color = nvgRGBA(0xFF, 0x00, 0x00, 0xFF);
    }

    text = music::ChordTable[pState->chords[pStep].chord].name;

}
// Chord menu

// Inversion menu
void InversionItem::onAction(const event::Action &e) {
    pChord->inversion = inversion;
    pChord->dirty = true;
}

void InversionChoice::onAction(const event::Action &e) {
    if (!pState)
        return;

    ui::Menu *menu = createMenu();
    menu->addChild(createMenuLabel("Inversion"));
    for (int i = 0; i < music::NUM_INV; i++) {
        InversionItem *item = new InversionItem;
        item->pChord = &(pState->chords[pStep]);
        item->inversion = i;
        item->text = music::inversionNames[i];
        menu->addChild(item);
    }
}

void InversionChoice::step() {
    if (!pState) {
        text = "";
        return;
    }

    if(pState->nSteps > pStep) {
        color = nvgRGBA(0x00, 0xFF, 0xFF, 0xFF);
    } else {
        color = nvgRGBA(0xFF, 0x00, 0x00, 0xFF);
    }

    text = music::inversionNames[pState->chords[pStep].inversion];

}
// Inversion menu

void KeyModeBox::step() {
    if (!pState) {
        text = "";
        return;
    }

    text = music::NoteDegreeModeNames[pState->key][0][pState->mode] + " " + music::modeNames[pState->mode];

}

// ProgressStepWidget
void ProgressStepWidget::setPState(ProgressState *pState, int pStep) {

    clearChildren();

    math::Vec pos;

    RootChoice *rootChoice = createWidget<RootChoice>(pos);
    rootChoice->box.size.x = 170.0;
    rootChoice->textOffset.y = 10.0;
    rootChoice->pState = pState;
    rootChoice->pStep = pStep;
    addChild(rootChoice);
    pos = rootChoice->box.getTopRight();
    this->rootChooser = rootChoice;

    this->separators[0] = createWidget<LedDisplaySeparator>(pos);
    this->separators[0]->box.size.x = box.size.x / 3.0;
    addChild(this->separators[0]);

    ChordChoice *chordChoice = createWidget<ChordChoice>(pos);
    chordChoice->box.size.x = 100.0;
    chordChoice->textOffset.y = 10.0;
    chordChoice->pState = pState;
    chordChoice->pStep = pStep;
    addChild(chordChoice);
    pos = chordChoice->box.getTopRight();
    this->chordChooser = chordChoice;

    this->separators[1] = createWidget<LedDisplaySeparator>(pos);
    this->separators[1]->box.size.x = box.size.x / 3.0;
    addChild(this->separators[1]);

    InversionChoice *inversionChoice = createWidget<InversionChoice>(pos);
    inversionChoice->box.size.x = 50.0;
    inversionChoice->textOffset.y = 10.0;
    inversionChoice->pState = pState;
    inversionChoice->pStep = pStep;
    addChild(inversionChoice);
    pos = inversionChoice->box.getTopRight();
    this->inversionChooser = inversionChoice;

}
// ProgressStepWidget

// ProgressStateWidget
void ProgressStateWidget::setPState(ProgressState *pState) {
    clearChildren();
    math::Vec pos;

    KeyModeBox *keyModeBox  = createWidget<KeyModeBox>(pos);
    keyModeBox->box.size.x = 170.0;
    keyModeBox->pState = pState;
    addChild(keyModeBox);
    pos = keyModeBox->box.getBottomLeft();

    for (int i = 0; i < 8; i++) {
        ProgressStepWidget *pWidget = createWidget<ProgressStepWidget>(pos);
        pWidget->box.size.x = box.size.x - 5;
        pWidget->box.size.y = box.size.y / 9.0;
        pWidget->setPState(pState, i);
        addChild(pWidget);
        pos = pWidget->box.getBottomLeft();
        this->stepConfig[i] = pWidget;
    }
}
// ProgressStateWidget

