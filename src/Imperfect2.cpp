#include "AH.hpp"
#include "AHCommon.hpp"

#include <iostream>

using namespace ah;

struct Imperfect2 : core::AHModule {

	enum ParamIds {
		ENUMS(DELAY_PARAM,4),
		ENUMS(DELAYSPREAD_PARAM,4),
		ENUMS(LENGTH_PARAM,4),
		ENUMS(LENGTHSPREAD_PARAM,4),
		ENUMS(DIVISION_PARAM,4),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(TRIG_INPUT,4),
		ENUMS(DELAY_INPUT,4),
		ENUMS(DELAYSPREAD_INPUT,4),
		ENUMS(LENGTH_INPUT,4),
		ENUMS(LENGTHSPREAD_INPUT,4),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUT,4),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(OUT_LIGHT,8),
		NUM_LIGHTS
	};

	Imperfect2() : core::AHModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {

		for (int i = 0; i < 4; i++) {
			configParam(DELAY_PARAM + i, 1.0f, 2.0f, 1.0f, "Delay length", "ms", -2.0f, 1000.0f, 0.0f);

			configParam(DELAYSPREAD_PARAM + i, 1.0f, 2.0f, 1.0f, "Delay length spread", "ms", -2.0f, 2000.0f, 0.0f);
			paramQuantities[DELAYSPREAD_PARAM + i]->description = "Magnitude of random time applied to delay length";

			configParam(LENGTH_PARAM + i, 1.001f, 2.0f, 1.001f, "Gate length", "ms", -2.0f, 1000.0f, 0.0f); // Always produce gate

			configParam(LENGTHSPREAD_PARAM + i, 1.0, 2.0, 1.0f, "Gate length spread", "ms", -2.0f, 2000.0f, 0.0f);
			paramQuantities[LENGTHSPREAD_PARAM + i]->description = "Magnitude of random time applied to gate length";

			configParam(DIVISION_PARAM + i, 1, 64, 1, "Clock division");

		}

		onReset();

	}
	
	void process(const ProcessArgs &args) override;
	
	void onReset() override {
		for (int i = 0; i < 4; i++) {
			delayState[i] = false;
			gateState[i] = false;
			delayTime[i] = 0.0;
			gateTime[i] = 0.0;
			bpm[i] = 0.0;
		}
	}

	bool delayState[4];
	bool gateState[4];
	float delayTime[4];
	int delayTimeMs[4];
	int delaySprMs[4];
	float gateTime[4];
	int gateTimeMs[4];
	int gateSprMs[4];
	float bpm[4];
	int division[4];
	int actDelayMs[4] = {0, 0, 0, 0};
	int actGateMs[4] = {0, 0, 0, 0};
	
	digital::AHPulseGenerator delayPhase[4];
	digital::AHPulseGenerator gatePhase[4];
	rack::dsp::SchmittTrigger inTrigger[4];

	int counter[4];
	
	digital::BpmCalculator bpmCalc[4];
		
};

void Imperfect2::process(const ProcessArgs &args) {
	
	AHModule::step();

	float dlyLen;
	float dlySpr;
	float gateLen;
	float gateSpr;
	
	int lastValidInput = -1;
	
	for (int i = 0; i < 4; i++) {
		
		bool generateSignal = false;
		
		bool inputActive = inputs[TRIG_INPUT + i].isConnected();
		bool haveTrigger = inTrigger[i].process(inputs[TRIG_INPUT + i].getVoltage());
		bool outputActive = outputs[OUT_OUTPUT + i].isConnected();
		
		// This is where we manage row-chaining/normalisation, i.e a row can be active without an
		// input by receiving the input clock from a previous (higher) row
		// If we have an active input, we should forget about previous valid inputs
		if (inputActive) {
			
			bpm[i] = bpmCalc[i].calculateBPM(args.sampleTime, inputs[TRIG_INPUT + i].getVoltage());
		
			lastValidInput = -1;
	
			if (haveTrigger) {
				if (debugEnabled()) { std::cout << stepX << " " << i << " has active input and has received trigger" << std::endl; }
				generateSignal = true;
				lastValidInput = i; // Row i has a valid input
			}
			
		} else {
			// We have an output plugged in this row and previously seen a trigger on previous row
			if (outputActive && lastValidInput > -1) {
				if (debugEnabled()) { std::cout << stepX << " " << i << " has active out and has seen trigger on " << lastValidInput << std::endl; }
				generateSignal = true;
			}
			
			bpm[i] = 0.0;
			
		}
		
		if (inputs[DELAY_INPUT + i].isConnected()) {
			dlyLen = log2(fabs(inputs[DELAY_INPUT + i].getVoltage()) + 1.0f); 
		} else {
			dlyLen = log2(params[DELAY_PARAM + i].getValue());
		}	

		if (inputs[DELAYSPREAD_INPUT + i].isConnected()) {
			dlySpr = log2(fabs(inputs[DELAYSPREAD_INPUT + i].getVoltage()) + 1.0f); 
		} else {
			dlySpr = log2(params[DELAYSPREAD_PARAM + i].getValue());
		}	
		
		if (inputs[LENGTH_INPUT + i].isConnected()) {
			gateLen = log2(fabs(inputs[LENGTH_INPUT + i].getVoltage()) + 1.001f); 
		} else {
			gateLen = log2(params[LENGTH_PARAM + i].getValue());
		}	
		
		if (inputs[LENGTHSPREAD_INPUT + i].isConnected()) {
			gateSpr = log2(fabs(inputs[LENGTHSPREAD_INPUT + i].getVoltage()) + 1.0f); 
		} else {
			gateSpr = log2(params[LENGTHSPREAD_PARAM + i].getValue());
		}	
		
		division[i] = params[DIVISION_PARAM + i].getValue();
		
		delayTimeMs[i] = dlyLen * 1000;
		delaySprMs[i] = dlySpr * 2000; // scaled by ±2 below
		gateTimeMs[i] = gateLen * 1000;
		gateSprMs[i] = gateSpr * 2000; // scaled by ±2 below
				
		if (generateSignal) {

			counter[i]++;
			int target = division[i];
		
			if (debugEnabled()) { 
				std::cout << stepX << " Div: " << i << ": Target: " << target << " Cnt: " << counter[lastValidInput] << " Exp: " << counter[lastValidInput] % division[i] << std::endl; 
			}

			if (counter[lastValidInput] % target == 0) { 

				// check that we are not in the gate phase
				if (!gatePhase[i].ishigh() && !delayPhase[i].ishigh()) {

				// Determine delay and gate times for all active outputs
					double rndD = clamp(random::normal(), -2.0f, 2.0f);
					delayTime[i] = clamp(dlyLen + dlySpr * rndD, 0.0f, 100.0f);
				
					// The modified gate time cannot be earlier than the start of the delay
					double rndG = clamp(random::normal(), -2.0f, 2.0f);
					gateTime[i] = clamp(gateLen + gateSpr * rndG, digital::TRIGGER, 100.0f);

					if (debugEnabled()) { 
						std::cout << stepX << " Delay: " << i << ": Len: " << dlyLen << " Spr: " << dlySpr << " r: " << rndD << " = " << delayTime[i] << std::endl; 
						std::cout << stepX << " Gate: " << i << ": Len: " << gateLen << ", Spr: " << gateSpr << " r: " << rndG << " = " << gateTime[i] << std::endl; 
					}

					// Trigger the respective delay pulse generator
					delayState[i] = true;
					if (delayPhase[i].trigger(delayTime[i])) {
						actDelayMs[i] = delayTime[i] * 1000;
					}
					
				}
			
			}
		}
	}
	
	for (int i = 0; i < 4; i++) {
	
		if (delayState[i] && !delayPhase[i].process(args.sampleTime)) {
			if (gatePhase[i].trigger(gateTime[i])) {
				actGateMs[i] = gateTime[i] * 1000;
			}
			gateState[i] = true;
			delayState[i] = false;
		}

		
		if (gatePhase[i].process(args.sampleTime)) {
			outputs[OUT_OUTPUT + i].setVoltage(10.0f);

			lights[OUT_LIGHT + i * 2].setSmoothBrightness(1.0f, args.sampleTime);
			lights[OUT_LIGHT + i * 2 + 1].setSmoothBrightness(0.0f, args.sampleTime);

		} else {
			outputs[OUT_OUTPUT + i].setVoltage(0.0f);
			gateState[i] = false;

			if (delayState[i]) {
				lights[OUT_LIGHT + i * 2].setSmoothBrightness(0.0f, args.sampleTime);
				lights[OUT_LIGHT + i * 2 + 1].setSmoothBrightness(1.0f, args.sampleTime);
			} else {
				lights[OUT_LIGHT + i * 2].setSmoothBrightness(0.0f, args.sampleTime);
				lights[OUT_LIGHT + i * 2 + 1].setSmoothBrightness(0.0f, args.sampleTime);
			}
			
		}
			
	}
	
}

struct Imperfect2Box : TransparentWidget {
	
	Imperfect2 *module;
	std::shared_ptr<Font> font;
	float *bpm;
	int *dly;
	int *dlySpr;
	int *gate;
	int *gateSpr;
	int *division;
	int *actDly;
	int *actGate;
	
	Imperfect2Box() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG14ClassicMini-BoldItalic.ttf"));
	}

	void draw(const DrawArgs &ctx) override {
	
		Vec pos = Vec(0, 15);

		nvgFontSize(ctx.vg, 10);
		nvgFontFaceId(ctx.vg, font->handle);
		nvgTextLetterSpacing(ctx.vg, -1);
		nvgTextAlign(ctx.vg, NVGalign::NVG_ALIGN_CENTER);
		nvgFillColor(ctx.vg, nvgRGBA(0x00, 0xFF, 0xFF, 0xFF));
	
		char text[10];
		if (*bpm == 0.0f) {
			snprintf(text, sizeof(text), "-");
		} else {
			snprintf(text, sizeof(text), "%.1f", *bpm);
		}
		nvgText(ctx.vg, pos.x + 20, pos.y, text, NULL);
		
		snprintf(text, sizeof(text), "%d", *dly);
		nvgText(ctx.vg, pos.x + 74, pos.y, text, NULL);

		if (*dlySpr != 0) {
			snprintf(text, sizeof(text), "%d", *dlySpr);
			nvgText(ctx.vg, pos.x + 144, pos.y, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", *gate);
		nvgText(ctx.vg, pos.x + 214, pos.y, text, NULL);

		if (*gateSpr != 0) {
			snprintf(text, sizeof(text), "%d", *gateSpr);
			nvgText(ctx.vg, pos.x + 284, pos.y, text, NULL);
		}

		snprintf(text, sizeof(text), "%d", *division);
		nvgText(ctx.vg, pos.x + 334, pos.y, text, NULL);
		
		nvgFillColor(ctx.vg, nvgRGBA(0, 0, 0, 0xff));
		snprintf(text, sizeof(text), "%d", *actDly);
		nvgText(ctx.vg, pos.x + 372, pos.y, text, NULL);

		snprintf(text, sizeof(text), "%d", *actGate);
		nvgText(ctx.vg, pos.x + 408, pos.y, text, NULL);

		
	}
	
};

struct Imperfect2Widget : ModuleWidget {

	Imperfect2Widget(Imperfect2 *module) {
	
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Imperfect2.svg")));

		for (int i = 0; i < 4; i++) {
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 0, i * 2 + 1, true, true), module, Imperfect2::TRIG_INPUT + i));
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 1, i * 2 + 1, true, true), module, Imperfect2::DELAY_INPUT + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 2, i * 2 + 1, true, true), module, Imperfect2::DELAY_PARAM + i));
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 3, i * 2 + 1, true, true), module, Imperfect2::DELAYSPREAD_INPUT + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 4, i * 2 + 1, true, true), module, Imperfect2::DELAYSPREAD_PARAM + i));
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 5, i * 2 + 1, true, true), module, Imperfect2::LENGTH_INPUT + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 6, i * 2 + 1, true, true), module, Imperfect2::LENGTH_PARAM + i)); 
			addInput(createInput<PJ301MPort>(gui::getPosition(gui::PORT, 7, i * 2 + 1, true, true), module, Imperfect2::LENGTHSPREAD_INPUT + i));
			addParam(createParam<gui::AHKnobNoSnap>(gui::getPosition(gui::KNOB, 8, i * 2 + 1, true, true), module, Imperfect2::LENGTHSPREAD_PARAM + i)); 
			addParam(createParam<gui::AHKnobSnap>(gui::getPosition(gui::KNOB, 9, i * 2 + 1, true, true), module, Imperfect2::DIVISION_PARAM + i));
			addChild(createLight<MediumLight<GreenRedLight>>(gui::getPosition(gui::LIGHT, 10, i * 2 + 1, true, true), module, Imperfect2::OUT_LIGHT + i * 2));
			addOutput(createOutput<PJ301MPort>(gui::getPosition(gui::PORT, 11, i * 2+ 1, true, true), module, Imperfect2::OUT_OUTPUT + i));
		}


		if (module != NULL) {

			{
				Imperfect2Box *display = createWidget<Imperfect2Box>(Vec(10, 95));

				display->module = module;
				display->box.size = Vec(200, 20);

				display->bpm = &(module->bpm[0]);
				display->dly = &(module->delayTimeMs[0]);
				display->dlySpr = &(module->delaySprMs[0]);
				display->gate = &(module->gateTimeMs[0]);
				display->gateSpr = &(module->gateSprMs[0]);
				display->division = &(module->division[0]);
				display->actDly = &(module->actDelayMs[0]);
				display->actGate = &(module->actGateMs[0]);
				
				addChild(display);
			}	

			{
				Imperfect2Box *display = createWidget<Imperfect2Box>(Vec(10, 165));

				display->module = module;
				display->box.size = Vec(200, 20);

				display->bpm = &(module->bpm[1]);
				display->dly = &(module->delayTimeMs[1]);
				display->dlySpr = &(module->delaySprMs[1]);
				display->gate = &(module->gateTimeMs[1]);
				display->gateSpr = &(module->gateSprMs[1]);
				display->division = &(module->division[1]);
				display->actDly = &(module->actDelayMs[1]);
				display->actGate = &(module->actGateMs[1]);
					
				addChild(display);
			}	

			{
				Imperfect2Box *display = createWidget<Imperfect2Box>(Vec(10, 235));

				display->module = module;
				display->box.size = Vec(200, 20);

				display->bpm = &(module->bpm[2]);
				display->dly = &(module->delayTimeMs[2]);
				display->dlySpr = &(module->delaySprMs[2]);
				display->gate = &(module->gateTimeMs[2]);
				display->gateSpr = &(module->gateSprMs[2]);
				display->division = &(module->division[2]);
				display->actDly = &(module->actDelayMs[2]);
				display->actGate = &(module->actGateMs[2]);
					
				addChild(display);
			}	

			{
				Imperfect2Box *display = createWidget<Imperfect2Box>(Vec(10, 305));

				display->module = module;
				display->box.size = Vec(200, 20);

				display->bpm = &(module->bpm[3]);
				display->dly = &(module->delayTimeMs[3]);
				display->dlySpr = &(module->delaySprMs[3]);
				display->gate = &(module->gateTimeMs[3]);
				display->gateSpr = &(module->gateSprMs[3]);
				display->division = &(module->division[3]);
				display->actDly = &(module->actDelayMs[3]);
				display->actGate = &(module->actGateMs[3]);
					
				addChild(display);
			}	
		}
	}
};

Model *modelImperfect2 = createModel<Imperfect2, Imperfect2Widget>("Imperfect2");

