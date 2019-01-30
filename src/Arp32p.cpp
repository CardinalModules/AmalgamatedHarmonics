#include "component.hpp"

#include "AH.hpp"
#include "Core.hpp"
#include "UI.hpp"

#include <iostream>


struct Pattern {
	
	int length = 0;
	int trans = 0;
	int scale = 0;
	int count = 0;
	int offset = 0;
	int end = 0;

	int MAJOR[7] = {0,2,4,5,7,9,11};
	int MINOR[7] = {0,2,3,5,7,8,10};
		
	virtual std::string getName() = 0;

	virtual void initialise(int l, int sc, int tr, int off) {
		length = l;
		trans = tr;
		scale = sc;
		offset = off;
	};

	virtual void advance() {
		count++;
	};

	virtual int getOffset() = 0;
	
	virtual bool isPatternFinished() = 0;
	
	int getMajor(int count) {
		int i = abs(count);
		int sign = (count < 0) ? -1 : (count > 0);
		return sign * ((i / 7) * 12 + MAJOR[i % 7]);
	}

	int getMinor(int count) {
		int i = abs(count);
		int sign = (count < 0) ? -1 : (count > 0);
		return sign * ((i / 7) * 12 + MINOR[i % 7]);
	}

};

struct DivergePattern : Pattern {

	std::string getName() override {
		return "Diverge";
	};

	void initialise(int l, int sc, int tr, int off) override {
		Pattern::initialise(l, sc, tr, off);
		end = length - 1;
		if (offset >= length - 1) {
			count = end;
		} else {
			count = offset;
		}
	}
	
	int getOffset() override {
		
		switch(scale) {
			case 0: return count * trans; break;
			case 1: return getMajor(count * trans); break;
			case 2: return getMinor(count * trans); break;
			default:
				return count * trans; break;
		}
	
	}

	bool isPatternFinished() override {
		return(count == end);
	}
	
};

struct ConvergePattern : Pattern {

	std::string getName() override {
		return "Converge";
	};	

	void initialise(int l, int sc, int tr, int off) override {
		Pattern::initialise(l, sc, tr, off);
		end = 0;
		if (offset >= length) {
			count = 0;
		} else {
			count = length - offset - 1;
		}
	} 
	
	void advance() override {
		count--;
	}
	
	int getOffset() override {
		switch(scale) {
			case 0: return -count * trans; break;
			case 1: return getMajor(-count * trans); break;
			case 2: return getMinor(-count * trans); break;
			default:
				return -count * trans; break;
		}
	}

	bool isPatternFinished() override {
		return (count == 0);
	}
	
};

struct ReturnPattern : Pattern {

	int mag = 0;
	
	std::string getName() override {
		return "Return";
	};	

	void initialise(int l, int sc, int tr, int off) override {
		Pattern::initialise(l, sc, tr, off);
		mag = length - 1;
		end = 2 * mag - 1;

		if (end < 1) {
			end = 1;
		}

		count = offset;

		if (count >= end) {
			count = end;
		}

	}
	
	int getOffset() override {
		
		if (length == 1) {
			return 0;
		}

		int note = (mag - abs(mag - count));
		
		switch(scale) {
			case 0: return note * trans; break;
			case 1: return getMajor(note * trans); break;
			case 2: return getMinor(note * trans); break;
			default:
				return note * trans; break;
		}

	}

	bool isPatternFinished() override {
		return(count == end);
	}
	
};

struct NotePattern : Pattern {

	std::vector<int> notes;
	
	void initialise(int l, int sc, int tr, int off) override {
		Pattern::initialise(l, sc, tr, off);
		count = off;
		if (count >= (int)notes.size()) {
			count = (int)notes.size();
		}
	}
	
	int getOffset() override {
		return notes[count];
	}

	bool isPatternFinished() override {
		return (count >= (int)notes.size());
	}
		
};

struct RezPattern : NotePattern {
	
	std::string getName() override {
		return "Rez";
	};	

	RezPattern() {
		notes.clear();
		notes.push_back(0);
		notes.push_back(12);
		notes.push_back(0);
		notes.push_back(0);
		notes.push_back(8);
		notes.push_back(0);
		notes.push_back(0);
		notes.push_back(3);		
		notes.push_back(0);
		notes.push_back(0);
		notes.push_back(3);
		notes.push_back(0);
		notes.push_back(3);
		notes.push_back(0);
		notes.push_back(8);
		notes.push_back(0);
	}
	
	
};

struct OnTheRunPattern : NotePattern {
	
	std::string getName() override {
		return "On The Run";
	};	

	OnTheRunPattern() {
		notes.clear();
		notes.push_back(0);
		notes.push_back(4);
		notes.push_back(6);
		notes.push_back(4);
		notes.push_back(9);
		notes.push_back(11);
		notes.push_back(13);		
		notes.push_back(11);		
	}
	
};

struct Arp32 : AHModule {
	
	const static int MAX_STEPS = 16;
	const static int MAX_DIST = 12; //Octave

	enum ParamIds {
		PATT_PARAM,
		LENGTH_PARAM,
		TRANS_PARAM,
		SCALE_PARAM,
		OFFSET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		PITCH_INPUT,
		PATT_INPUT,
		LENGTH_INPUT,
		TRANS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		GATE_OUTPUT,
		EOC_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	Arp32() : AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		params[PATT_PARAM].config(0.0, 4.0, 0.0); 
		params[TRANS_PARAM].config(-24, 24, 1); 
		params[LENGTH_PARAM].config(1.0, 16.0, 1.0); 
		params[OFFSET_PARAM].config(0.0, 10.0, 0.0); 
		params[SCALE_PARAM].config(0, 2, 0); 

		onReset();
		id = rand();
        debugFlag = false;
	}

	void step() override;
	
	void onReset() override {
		isRunning = false;
	}
	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// gateMode
		json_t *gateModeJ = json_integer((int) gateMode);
		json_object_set_new(rootJ, "gateMode", gateModeJ);

		return rootJ;
	}
	
	void dataFromJson(json_t *rootJ) override {
		// gateMode
		json_t *gateModeJ = json_object_get(rootJ, "gateMode");
		
		if (gateModeJ) {
			gateMode = (GateMode)json_integer_value(gateModeJ);
		}
	}
	
	enum GateMode {
		TRIGGER,
		RETRIGGER,
		CONTINUOUS,
	};
	GateMode gateMode = TRIGGER;
	
	dsp::SchmittTrigger clockTrigger; // for clock
	
	dsp::PulseGenerator gatePulse;
	dsp::PulseGenerator eocPulse;

	float outVolts = 0;
	float rootPitch = 0.0;
	bool isRunning = false;
	int error = 0;
	
	int inputPat = 0;
	int inputLen = 0;
	int inputTrans = 0;
	int inputScale = 0;
	
	int pattern = 0;
	int length = 0;
	float trans = 0;
	float scale = 0;

	int poll = 5000;
		
	float semiTone = 1.0 / 12.0;

	DivergePattern			patt_diverge; 
	ConvergePattern 		patt_converge; 
	ReturnPattern 			patt_return;
	RezPattern 				patt_rez;
	OnTheRunPattern			patt_ontherun;

	DivergePattern			ui_patt_diverge; 
	ConvergePattern 		ui_patt_converge; 
	ReturnPattern 			ui_patt_return;
	RezPattern 				ui_patt_rez;
	OnTheRunPattern			ui_patt_ontherun;

	Pattern *currPatt = &patt_diverge;
	Pattern *uiPatt = &ui_patt_diverge;
	
	int id = 0;

};


void Arp32::step() {
	
	AHModule::step();

	// Wait a few steps for the inputs to flow through Rack
	if (stepX < 10) { 
		return;
	}
	
	// Get inputs from Rack
	float clockInput	= inputs[CLOCK_INPUT].value;
	float clockActive	= inputs[CLOCK_INPUT].active;
	
	// Read param section	
	if (inputs[PATT_INPUT].active) {
		inputPat = inputs[PATT_INPUT].value;
	} else {
		inputPat = params[PATT_PARAM].value;
	}	

	if (inputs[LENGTH_INPUT].active) {
		inputLen = inputs[LENGTH_INPUT].value;
	} else {
		inputLen = params[LENGTH_PARAM].value;
	}	
	
	if (inputs[TRANS_INPUT].active) {
		inputTrans = inputs[TRANS_INPUT].value;
	} else {
		inputTrans = params[TRANS_PARAM].value;
	}	

	inputScale = params[SCALE_PARAM].value;

	int offset = params[OFFSET_PARAM].value;

	// Process inputs
	bool clockStatus	= clockTrigger.process(clockInput);
	
	// Need to understand why this happens
	if (inputLen == 0) {
		if (debugEnabled()) { std::cout << stepX << " " << id  << " InputLen == 0, aborting" << std::endl; }
		return; // No inputs, no music
	}
	
	// If there is no clock input, then force that we are not running
	if (!clockActive) {
		isRunning = false;
	}
				
	bool restart = false;

	// Have we been clocked?
	if (clockStatus) {
		
		// If we are already running, process cycle
		if (isRunning) {

			if (debugEnabled()) { std::cout << stepX << " " << id  << " Advance Cycle: " << currPatt->getOffset() << std::endl; }

			// Reached the end of the pattern?
			if (currPatt->isPatternFinished()) {

				// Pulse the EOC gate
				eocPulse.trigger(Core::TRIGGER);

				if (debugEnabled()) { std::cout << stepX << " " << id  << " Finished Cycle" << std::endl; }
				restart = true;

			} 
							
			// Finally set the out voltage
			outVolts = clamp(rootPitch + semiTone * (float)currPatt->getOffset(), -10.0f, 10.0f);
			
			if (debugEnabled()) { std::cout << stepX << " " << id  << " Output V = " << outVolts << std::endl; }
					
			// Pulse the output gate
			gatePulse.trigger(Core::TRIGGER);

			// Completed 1 step
			currPatt->advance();

		} else {

			// Start a cycle
			restart = true;

		}

	}
	
	// If we have been triggered, start a new sequence
	if (restart) {
		
		// Read input pitches and assign to pitch array
		float inputPitch;
		if (inputs[PITCH_INPUT].active) {
			inputPitch = inputs[PITCH_INPUT].value;
		} else {
			inputPitch = 0.0;
		}

		// At the first step of the cycle
		// So this is where we tweak the cycle parameters
		pattern = inputPat;
		length = inputLen;
		trans = inputTrans;
		scale = inputScale;
		
		switch(pattern) {
			case 0:		currPatt = &patt_diverge; 		break;
			case 1:		currPatt = &patt_converge;		break;
			case 2:		currPatt = &patt_return;		break;
			case 3:		currPatt = &patt_rez;			break;
			case 4:		currPatt = &patt_ontherun;		break;
			default:	currPatt = &patt_diverge;		break;
		};

		// Save pitch
		rootPitch = inputPitch;

		if (debugEnabled()) { std::cout << stepX << " " << id  << 
			" Initiatise new Cycle: Pattern: " << currPatt->getName() << 
			" Length: " << inputLen << std::endl; 
		}
		
		currPatt->initialise(length, scale, trans, offset);

		// Start
		isRunning = true;
		
	} 
	
	// Update UI
	switch(inputPat) {
		case 0:		uiPatt = &ui_patt_diverge; 		break;
		case 1:		uiPatt = &ui_patt_converge;		break;
		case 2:		uiPatt = &ui_patt_return;		break;
		case 3:		uiPatt = &ui_patt_rez;			break;
		case 4:		uiPatt = &ui_patt_ontherun;		break;
		default:	uiPatt = &ui_patt_diverge;		break;
	};

	// Initialise the pattern
	uiPatt->initialise(inputLen, inputScale, inputTrans, offset);

	// Set the value
	outputs[OUT_OUTPUT].value = outVolts;
	
	bool gPulse = gatePulse.process(delta);
	bool cPulse = eocPulse.process(delta);
	
	bool gatesOn = isRunning;
	if (gateMode == TRIGGER) {
		gatesOn = gatesOn && gPulse;
	} else if (gateMode == RETRIGGER) {
		gatesOn = gatesOn && !gPulse;
	}
	
	outputs[GATE_OUTPUT].value = gatesOn ? 10.0 : 0.0;
	outputs[EOC_OUTPUT].value = cPulse ? 10.0 : 0.0;
	
}

struct Arp32Display : TransparentWidget {
	
	Arp32 *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	Arp32Display() {
		font = Font::load(asset::plugin(pluginInstance, "res/EurostileBold.ttf"));
	}

	void draw(const DrawContext &ctx) override {
	
		Vec pos = Vec(0, 15);

		nvgFontSize(ctx.vg, 14);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);

		nvgFillColor(ctx.vg, nvgRGBA(255, 0, 0, 0xff));
	
		char text[128];
		if (module->inputLen == 0) {
			snprintf(text, sizeof(text), "Error: inputLen == 0");
			nvgText(ctx.vg, pos.x + 10, pos.y, text, NULL);			
		} else {
			snprintf(text, sizeof(text), "%s", module->uiPatt->getName().c_str()); 
			nvgText(ctx.vg, pos.x + 10, pos.y, text, NULL);			
			snprintf(text, sizeof(text), "L : %d", module->uiPatt->length); 
			nvgText(ctx.vg, pos.x + 10, pos.y + 15, text, NULL);
			switch(module->uiPatt->scale) {
				case 0: 
					snprintf(text, sizeof(text), "S : %dst", module->uiPatt->trans); 
					break;
				case 1: 
					snprintf(text, sizeof(text), "S : %dM", module->uiPatt->trans); 
					break;
				case 2: 
					snprintf(text, sizeof(text), "S : %dm", module->uiPatt->trans); 
					break;
				default: snprintf(text, sizeof(text), "Error..."); break;
			}
			nvgText(ctx.vg, pos.x + 60, pos.y + 15, text, NULL);

		}
	}
	
};

struct Arp32Widget : ModuleWidget {

	Arp32Widget(Arp32 *module) {
		
		setModule(module);
		setPanel(SVG::load(asset::plugin(pluginInstance, "res/Arp32p.svg")));
		UI ui;

		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 0, true, false), module, Arp32::PITCH_INPUT));
		
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 0, 2, true, false), module, Arp32::PATT_PARAM)); 
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 3, true, false), module, Arp32::PATT_INPUT));
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 1, 2, true, false), module, Arp32::TRANS_PARAM)); 
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 3, true, false), module, Arp32::TRANS_INPUT)); 
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 2, 2, true, false), module, Arp32::LENGTH_PARAM)); 
		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 3, true, false), module, Arp32::LENGTH_INPUT));

		addInput(createInput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 4, true, false), module, Arp32::CLOCK_INPUT));
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 1, 4, true, false), module, Arp32::OFFSET_PARAM)); 
		addParam(createParam<AHKnobSnap>(ui.getPosition(UI::KNOB, 2, 4, true, false), module, Arp32::SCALE_PARAM)); 

		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 0, 5, true, false), module, Arp32::OUT_OUTPUT));
		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 1, 5, true, false), module, Arp32::GATE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(ui.getPosition(UI::PORT, 2, 5, true, false), module, Arp32::EOC_OUTPUT));

		if (module != NULL) {
			Arp32Display *displayW = createWidget<Arp32Display>(Vec(10, 90));
			displayW->box.size = Vec(100, 140);
			displayW->module = module;
			addChild(displayW);
		}

	}

	void appendContextMenu(Menu *menu) override {

		Arp32 *arp = dynamic_cast<Arp32*>(module);
		assert(arp);

		struct GateModeItem : MenuItem {
			Arp32 *module;
			Arp32::GateMode gateMode;
			void onAction(const event::Action &e) override {
				module->gateMode = gateMode;
			}
		};

		struct GateModeMenu : MenuItem {
			Arp32 *module;
			Menu *createChildMenu() override {
				Menu *menu = new Menu;
				std::vector<Arp32::GateMode> modes = {Arp32::TRIGGER, Arp32::RETRIGGER, Arp32::CONTINUOUS};
				std::vector<std::string> names = {"Trigger", "Retrigger", "Continuous"};
				for (size_t i = 0; i < modes.size(); i++) {
					GateModeItem *item = createMenuItem<GateModeItem>(names[i], CHECKMARK(module->gateMode == modes[i]));
					item->module = module;
					item->gateMode = modes[i];
					menu->addChild(item);
				}
				return menu;
			}
		};

		menu->addChild(construct<MenuLabel>());
		GateModeMenu *item = createMenuItem<GateModeMenu>("Gate Mode");
		item->module = arp;
		menu->addChild(item);

     }
	 
};

Model *modelArp32 = createModel<Arp32, Arp32Widget>("Arp32");

