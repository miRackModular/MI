//***********************************************************************************************
//
//SuperDriveFx module for VCV Rack by Alfredo Santamaria  - AS - https://github.com/AScustomWorks/AS
//Variable-hardness clipping code from scoofy[ AT ]inf[ DOT ]elte[ DOT ]hu
//Filter code from from VCV rack dsp
//
//***********************************************************************************************

#include "AS.hpp"
/*
#include "dsp/digital.hpp"
#include "dsp/filter.hpp"
*/
//#include <stdlib.h>

struct SuperDriveFx : Module{
	enum ParamIds {
		DRIVE_PARAM,
		OUTPUT_GAIN_PARAM,
		TONE_PARAM,
        BYPASS_SWITCH,
		NUM_PARAMS
	};
	enum InputIds {
		SIGNAL_INPUT,
		DRIVE_CV_INPUT,
		GAIN_CV_INPUT,
		TONE_CV_INPUT,
		BYPASS_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		GAIN_LIGHT,
		TONE_LIGHT,
		DRIVE_LIGHT,
		BYPASS_LED,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger bypass_button_trig;
	dsp::SchmittTrigger bypass_cv_trig;

	int drive_scale=50;//to handle cv parameters properly

	dsp::RCFilter lowpassFilter;
	dsp::RCFilter highpassFilter;

	bool fx_bypass = false;

	float fade_in_fx = 0.0f;
	float fade_in_dry = 0.0f;
	float fade_out_fx = 1.0f;
	float fade_out_dry = 1.0f;
    const float fade_speed = 0.001f;

	float input_signal=0.0f;
	float drive = 0.1f;
	float process_fx= 0.0f;
	float inv_atan_drive = 0.0f;
	float output_signal= 0.0f;

	SuperDriveFx() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(SuperDriveFx::DRIVE_PARAM, 0.0f, 1.0f, 0.0f, "Drive", "%", 0.0f, 100.0f);
		configParam(SuperDriveFx::TONE_PARAM, 0.0f, 1.0f, 0.5f, "Tone", "%", 0.0f, 100.0f);
		configParam(SuperDriveFx::OUTPUT_GAIN_PARAM, 0.0f, 1.0f, 0.5f, "Gain", "%", 0.0f, 100.0f);
		configParam(SuperDriveFx::BYPASS_SWITCH , 0.0f, 1.0f, 0.0f, "Bypass");	
				
	}

	void resetFades(){
		fade_in_fx = 0.0f;
		fade_in_dry = 0.0f;
		fade_out_fx = 1.0f;
		fade_out_dry = 1.0f;
	}

	void process(const ProcessArgs &args) override {

		if (bypass_button_trig.process(params[BYPASS_SWITCH].getValue()) || bypass_cv_trig.process(inputs[BYPASS_CV_INPUT].getVoltage()) ){
			fx_bypass = !fx_bypass;
			resetFades();
		}
		lights[BYPASS_LED].value = fx_bypass ? 1.0f : 0.0f;

		float input_signal = inputs[SIGNAL_INPUT].getVoltage();
		//OVERDRIVE SIGNAL
		//float drive = params[DRIVE_PARAM].getValue();
		
		drive = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.0f, 0.1f, 1.0f);
		//drive = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.0f, 0.1f, 1.0f);

		drive = drive * drive_scale;
		//precalc
		inv_atan_drive = 1.0f/atan(drive);
		//process
		process_fx = inv_atan_drive * atan(input_signal*drive);
		//output_signal = process_fx * params[OUTPUT_GAIN_PARAM].getValue();
		output_signal = process_fx * clamp(params[OUTPUT_GAIN_PARAM].getValue() + inputs[GAIN_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);

		//TONE CONTROL
		float tone = clamp(params[TONE_PARAM].getValue() + inputs[TONE_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		float lowpassFreq = 10000.0f * powf(10.0f, clamp(2.0f*tone, 0.0f, 1.0f));
		lowpassFilter.setCutoff(lowpassFreq / APP->engine->getSampleRate());
		lowpassFilter.process(output_signal);
		output_signal = lowpassFilter.lowpass();
		float highpassFreq = 10.0f * powf(100.0f, clamp(2.0f*tone - 1.0f, 0.0f, 1.0f));
		highpassFilter.setCutoff(highpassFreq / APP->engine->getSampleRate());
		highpassFilter.process(output_signal);
		output_signal = highpassFilter.highpass();

		//check bypass switch status
		if (fx_bypass){
			fade_in_dry += fade_speed;
			if ( fade_in_dry > 1.0f ) {
				fade_in_dry = 1.0f;
			}
			fade_out_fx -= fade_speed;
			if ( fade_out_fx < 0.0f ) {
				fade_out_fx = 0.0f;
			}
			outputs[SIGNAL_OUTPUT].setVoltage(( input_signal * fade_in_dry ) + ( (output_signal*3.5f) * fade_out_fx ));
		}else{
			fade_in_fx += fade_speed;
			if ( fade_in_fx > 1.0f ) {
				fade_in_fx = 1.0f;
			}
			fade_out_dry -= fade_speed;
			if ( fade_out_dry < 0.0f ) {
				fade_out_dry = 0.0f;
			}
			outputs[SIGNAL_OUTPUT].setVoltage(( input_signal * fade_out_dry ) + ( (output_signal*3.5f) * fade_in_fx ));
		}

		lights[DRIVE_LIGHT].value = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		lights[TONE_LIGHT].value = clamp(params[TONE_PARAM].getValue() + inputs[TONE_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		lights[GAIN_LIGHT].value = clamp(params[OUTPUT_GAIN_PARAM].getValue() + inputs[GAIN_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);

	}

	json_t *dataToJson()override {
		json_t *rootJm = json_object();

		json_t *statesJ = json_array();
		
			json_t *bypassJ = json_boolean(fx_bypass);
			json_array_append_new(statesJ, bypassJ);
		
		json_object_set_new(rootJm, "as_FxBypass", statesJ);

		return rootJm;
	}

	void dataFromJson(json_t *rootJm)override {
		json_t *statesJ = json_object_get(rootJm, "as_FxBypass");
		
			json_t *bypassJ = json_array_get(statesJ, 0);

			fx_bypass = !!json_boolean_value(bypassJ);
		
	}
	
};


struct SuperDriveFxWidget : ModuleWidget { 

	SuperDriveFxWidget(SuperDriveFx *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SuperDrive.svg")));
		dynamic_cast<SVGPanel*>(panel)->setBorderColor(nvgRGB(0x36, 0x61, 0x7c));
	
		//SCREWS
		addChild(createWidget<as_HexScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<as_HexScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<as_HexScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<as_HexScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		//KNOBS  
		addParam(createParam<as_KnobSmall>(Vec(8, 50), module, SuperDriveFx::DRIVE_PARAM));
		addParam(createParam<as_KnobSmall>(Vec(51, 50), module, SuperDriveFx::TONE_PARAM));
		addParam(createParam<as_KnobSmall>(Vec(8, 145), module, SuperDriveFx::OUTPUT_GAIN_PARAM));
		//LIGHTS
		addChild(createLight<SmallLight<YellowLight>>(Vec(30, 81.5), module, SuperDriveFx::DRIVE_LIGHT));
		addChild(createLight<SmallLight<YellowLight>>(Vec(73, 81.5), module, SuperDriveFx::TONE_LIGHT));
		addChild(createLight<SmallLight<YellowLight>>(Vec(30, 176.5), module, SuperDriveFx::GAIN_LIGHT));
		//BYPASS SWITCH
		addParam(createParam<LEDBezel>(Vec(55, 149), module, SuperDriveFx::BYPASS_SWITCH ));
		addChild(createLight<LedLight<RedLight>>(Vec(55+2.2, 149+2.2), module, SuperDriveFx::BYPASS_LED));
		//INS/OUTS
		addInput(createInput<as_PJ301MPort>(Vec(11, 291), module, SuperDriveFx::SIGNAL_INPUT));
		addOutput(createOutput<as_PJ301MPortGold>(Vec(54, 291), module, SuperDriveFx::SIGNAL_OUTPUT));
		//CV INPUTS
		addInput(createInput<as_PJ301MPort>(Vec(11, 89), module, SuperDriveFx::DRIVE_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(54, 89), module, SuperDriveFx::TONE_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(11, 184), module, SuperDriveFx::GAIN_CV_INPUT));

		//BYPASS CV INPUT
		addInput(createInput<as_PJ301MPort>(Vec(54, 184), module, SuperDriveFx::BYPASS_CV_INPUT));
	
	}
};

Model *modelSuperDriveFx = createModel<SuperDriveFx, SuperDriveFxWidget>("SuperDriveFx");