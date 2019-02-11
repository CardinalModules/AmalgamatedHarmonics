#include "ProgressState.hpp"

// ProgressState
ProgressState::ProgressState() {

    int *chordArray = music::ChordTable[1].root;

    for (int i = 0; i < 8; i++) {
        chords[i].degree = 0;
        chords[i].quality = 0;
        chords[i].chord = 1;
        chords[i].root = 0;
        chords[i].inversion = 0;

        for (int j = 0; j < 6; j++) {
            if (chordArray[j] < 0) {
                int off = offset;
                if (offset == 0) { // if offset = 0, randomise offset per note
                    off = (rand() % 3 + 1) * 12;
                }
                chords[i].pitches[j] = music::getVoltsFromPitch(chordArray[j] + off,chords[i].root);			
            } else {
                chords[i].pitches[j] = music::getVoltsFromPitch(chordArray[j],      chords[i].root);
            }	
        }
    }
}

void ProgressState::setMode(int m) {
    if (mode != m) {
        mode = m;
        if(chordMode) { 
            for (int i = 0; i < 8; i++) {
                music::getRootFromMode(mode, key, chords[i].degree, &(chords[i].root), &(chords[i].quality));
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
                music::getRootFromMode(mode, key, chords[i].degree, &(chords[i].root), &(chords[i].quality));
            }
        }
        dirty = true;
    }
}
// ProgressState

// Root/Degree menu
void RootItem::onAction(const event::Action &e) {
    pChord->root = root;
    pChord->dirty = true;
}

void DegreeItem::onAction(const event::Action &e) {
    pChord->degree = degree;
    pChord->dirty = true;

    // Root and chord can get updated
    // From the input root, mode and degree, we can get the root chord note and quality (Major,Minor,Diminshed)
    music::getRootFromMode(pState->mode, pState->key, pChord->degree, &(pChord->root), &(pChord->quality));

    // // Now get the actual chord from the main list
    // switch(pState->chords[step].quality) {
    // 	case music::MAJ: 
    // 		pState->chords[step].chord = round(rescale(fabs(pState->chords[step].inQuality), 0.0f, 10.0f, 1.0f, 70.0f)); 
    // 		break;
    // 	case music::MIN: 
    // 		pState->chords[step].chord = round(rescale(fabs(pState->chords[step].inQuality), 0.0f, 10.0f, 71.0f, 90.0f));
    // 		break;
    // 	case music::DIM: 
    // 		pState->chords[step].chord = round(rescale(fabs(pState->chords[step].inQuality), 0.0f, 10.0f, 91.0f, 98.0f));
    // 		break;		
    // }
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

    if (pState->chordMode) {
        text = music::NoteDegreeModeNames[pState->chords[pStep].root][pState->chords[pStep].degree][pState->mode];
        int index = pState->chords[pStep].degree * 3 + pState->chords[pStep].quality;
        text += " " + music::degreeNames[index];
    } else {
        text = music::noteNames[pState->chords[pStep].root];
    }
}
// Root/Degree menu

// Chord menu
void ChordItem::onAction(const event::Action &e)  {
    pChord->chord = chord;
    pChord->dirty = true;
}

void ChordChoice::onAction(const event::Action &e) {
    if (!pState)
        return;

    ui::Menu *menu = createMenu();
    menu->addChild(createMenuLabel("Chord"));
    for (int i = 1; i < music::NUM_CHORDS; i++) {
        ChordItem *item = new ChordItem;
        item->pChord = &(pState->chords[pStep]);
        item->chord = i;
        item->text = music::ChordTable[i].name;
        menu->addChild(item);
    }
}

void ChordChoice::step() {
    if (!pState) {
        text = "";
        return;
    }

    text = music::ChordTable[pState->chords[pStep].chord].name;

    if (text.empty()) {
        text = "()";
        color.a = 0.5f;
    } else {
        color.a = 1.f;
    }
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
    text = music::inversionNames[pState->chords[pStep].inversion];
    if (text.empty()) {
        text = "()";
        color.a = 0.5f;
    }
    else {
        color.a = 1.f;
    }
}
// Inversion menu

// ProgressStepWidget
void ProgressStepWidget::setPState(ProgressState *pState, int pStep) {

    clearChildren();

    math::Vec pos;

    RootChoice *rootChoice = createWidget<RootChoice>(pos);
    rootChoice->box.size.x = box.size.x;
    rootChoice->pState = pState;
    rootChoice->pStep = pStep;
    addChild(rootChoice);
    pos = rootChoice->box.getBottomLeft();
    this->rootChooser = rootChoice;

    this->separators[0] = createWidget<LedDisplaySeparator>(pos);
    this->separators[0]->box.size.x = box.size.x;
    addChild(this->separators[0]);

    ChordChoice *chordChoice = createWidget<ChordChoice>(pos);
    chordChoice->box.size.x = box.size.x;
    chordChoice->pState = pState;
    chordChoice->pStep = pStep;
    addChild(chordChoice);
    pos = chordChoice->box.getBottomLeft();
    this->chordChooser = chordChoice;

    this->separators[1] = createWidget<LedDisplaySeparator>(pos);
    this->separators[1]->box.size.x = box.size.x;
    addChild(this->separators[1]);

    InversionChoice *inversionChoice = createWidget<InversionChoice>(pos);
    inversionChoice->box.size.x = box.size.x;
    inversionChoice->pState = pState;
    inversionChoice->pStep = pStep;
    addChild(inversionChoice);
    pos = inversionChoice->box.getBottomLeft();
    this->inversionChooser = inversionChoice;

}
// ProgressStepWidget

// ProgressStateWidget
void ProgressStateWidget::setPState(ProgressState *pState) {
    clearChildren();
    math::Vec pos;
    for (int i = 0; i < 8; i++) {
        ProgressStepWidget *pWidget = createWidget<ProgressStepWidget>(pos);
        pWidget->box.size.x = (box.size.x - 5) / 8.0;
        pWidget->setPState(pState, i);
        addChild(pWidget);
        pos = pWidget->box.getTopRight();
        this->stepConfig[i] = pWidget;
    }
}
// ProgressStateWidget

