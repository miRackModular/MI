//***********************************************************************************************
//
//SuperDriveStereoFx module for VCV Rack by Alfredo Santamaria  - AS - https://github.com/AScustomWorks/AS
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

struct SuperDriveStereoFx : Module{
	enum ParamIds {
		DRIVE_PARAM,
		OUTPUT_GAIN_PARAM,
		TONE_PARAM,
        BYPASS_SWITCH,
		NUM_PARAMS
	};
	enum InputIds {
		SIGNAL_INPUT_L,
		SIGNAL_INPUT_R,
		DRIVE_CV_INPUT,
		GAIN_CV_INPUT,
		TONE_CV_INPUT,
		BYPASS_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIGNAL_OUTPUT_L,
		SIGNAL_OUTPUT_R,
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

	dsp::RCFilter lowpassFilterL;
	dsp::RCFilter highpassFilterL;
	dsp::RCFilter lowpassFilterR;
	dsp::RCFilter highpassFilterR;

	bool fx_bypass = false;

	float fade_in_fx = 0.0f;
	float fade_in_dry = 0.0f;
	float fade_out_fx = 1.0f;
	float fade_out_dry = 1.0f;
    const float fade_speed = 0.001f;

	float drive = 0.1f;
	float process_fx_L= 0.0f;
	float process_fx_R= 0.0f;
	float inv_atan_drive = 0.0f;

	float input_signal_L = 0.0f;
	float output_signal_L = 0.0f;
	float input_signal_R = 0.0f;
	float output_signal_R = 0.0f;

	bool mono_signal = false;


	SuperDriveStereoFx() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(SuperDriveStereoFx::DRIVE_PARAM, 0.0f, 1.0f, 0.0f, "Drive", "%", 0.0f, 100.0f);
		configParam(SuperDriveStereoFx::TONE_PARAM, 0.0f, 1.0f, 0.5f, "Tone", "%", 0.0f, 100.0f);
		configParam(SuperDriveStereoFx::OUTPUT_GAIN_PARAM, 0.0f, 1.0f, 0.5f, "Gain", "%", 0.0f, 100.0f);
		configParam(SuperDriveStereoFx::BYPASS_SWITCH , 0.0f, 1.0f, 0.0f, "Bypass");	
				
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

		//float input_signal = inputs[SIGNAL_INPUT].getVoltage();

		input_signal_L = clamp(inputs[SIGNAL_INPUT_L].getVoltage(),-10.0f,10.0f);

		if(!inputs[SIGNAL_INPUT_R].isConnected()){
			mono_signal = true;
			input_signal_R = input_signal_L;
		}else{
			mono_signal = false;
			input_signal_R = clamp(inputs[SIGNAL_INPUT_R].getVoltage(),-10.0f,10.0f);
		}
		//OVERDRIVE SIGNAL
		
		drive = clamp(params[DRIVE_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.0f, 0.1f, 1.0f);

		drive = drive * drive_scale;
		//precalc
		inv_atan_drive = 1.0f/atan(drive);
		//process
		process_fx_L = inv_atan_drive * atan(input_signal_L*drive);
		output_signal_L = process_fx_L * clamp(params[OUTPUT_GAIN_PARAM].getValue() + inputs[GAIN_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		if(mono_signal){
			output_signal_R = output_signal_L;
		}else{
			process_fx_R = inv_atan_drive * atan(input_signal_R*drive);
			output_signal_R = process_fx_R * clamp(params[OUTPUT_GAIN_PARAM].getValue() + inputs[GAIN_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		}

		//TONE CONTROL
		float tone = clamp(params[TONE_PARAM].getValue() + inputs[TONE_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
		float lowpassFreq = 10000.0f * powf(10.0f, clamp(2.0f*tone, 0.0f, 1.0f));
		lowpassFilterL.setCutoff(lowpassFreq / APP->engine->getSampleRate());

		lowpassFilterL.process(output_signal_L);
		output_signal_L = lowpassFilterL.lowpass();

		if(mono_signal){
			output_signal_R = output_signal_L;
		}else{
			lowpassFilterR.setCutoff(lowpassFreq / APP->engine->getSampleRate());
			lowpassFilterR.process(output_signal_R);
			output_signal_R = lowpassFilterR.lowpass();
		}

		float highpassFreq = 10.0f * powf(100.0f, clamp(2.0f*tone - 1.0f, 0.0f, 1.0f));
		highpassFilterL.setCutoff(highpassFreq / APP->engine->getSampleRate());

		highpassFilterL.process(output_signal_L);
		output_signal_L = highpassFilterL.highpass();

		if(mono_signal){
			output_signal_R = output_signal_L;
		}else{
			highpassFilterR.setCutoff(highpassFreq / APP->engine->getSampleRate());
			highpassFilterR.process(output_signal_R);
			output_signal_R = highpassFilterR.highpass();
		}


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
			outputs[SIGNAL_OUTPUT_L].setVoltage(( input_signal_L * fade_in_dry ) + ( (output_signal_L*3.5f) * fade_out_fx ));
			outputs[SIGNAL_OUTPUT_R].setVoltage(( input_signal_R * fade_in_dry ) + ( (output_signal_R*3.5f) * fade_out_fx ));

		}else{
			fade_in_fx += fade_speed;
			if ( fade_in_fx > 1.0f ) {
				fade_in_fx = 1.0f;
			}
			fade_out_dry -= fade_speed;
			if ( fade_out_dry < 0.0f ) {
				fade_out_dry = 0.0f;
			}
			outputs[SIGNAL_OUTPUT_L].setVoltage(( input_signal_L * fade_out_dry ) + ( (output_signal_L*3.5f) * fade_in_fx ));
			outputs[SIGNAL_OUTPUT_R].setVoltage(( input_signal_R * fade_out_dry ) + ( (output_signal_R*3.5f) * fade_in_fx ));
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


struct SuperDriveStereoFxWidget : ModuleWidget { 

	SuperDriveStereoFxWidget(SuperDriveStereoFx *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SuperDriveStereo.svg")));
		dynamic_cast<SVGPanel*>(panel)->setBorderColor(nvgRGB(0x36, 0x61, 0x7c));
	
		//SCREWS
		addChild(createWidget<as_HexScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<as_HexScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<as_HexScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<as_HexScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		//KNOBS  
		addParam(createParam<as_KnobSmall>(Vec(8, 50), module, SuperDriveStereoFx::DRIVE_PARAM));
		addParam(createParam<as_KnobSmall>(Vec(51, 50), module, SuperDriveStereoFx::TONE_PARAM));
		addParam(createParam<as_KnobSmall>(Vec(8, 145), module, SuperDriveStereoFx::OUTPUT_GAIN_PARAM));
		//LIGHTS
		addChild(createLight<SmallLight<YellowLight>>(Vec(30, 81.5), module, SuperDriveStereoFx::DRIVE_LIGHT));
		addChild(createLight<SmallLight<YellowLight>>(Vec(73, 81.5), module, SuperDriveStereoFx::TONE_LIGHT));
		addChild(createLight<SmallLight<YellowLight>>(Vec(30, 176.5), module, SuperDriveStereoFx::GAIN_LIGHT));
		//CV INPUTS
		addInput(createInput<as_PJ301MPort>(Vec(11, 89), module, SuperDriveStereoFx::DRIVE_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(54, 89), module, SuperDriveStereoFx::TONE_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(11, 184), module, SuperDriveStereoFx::GAIN_CV_INPUT));
		//BYPASS SWITCH
		addParam(createParam<LEDBezel>(Vec(55, 149), module, SuperDriveStereoFx::BYPASS_SWITCH ));
		addChild(createLight<LedLight<RedLight>>(Vec(55+2.2, 149+2), module, SuperDriveStereoFx::BYPASS_LED));
		//BYPASS CV INPUT
		addInput(createInput<as_PJ301MPort>(Vec(54, 184), module, SuperDriveStereoFx::BYPASS_CV_INPUT));
		//INPUTS
		addInput(createInput<as_PJ301MPort>(Vec(11, 291), module, SuperDriveStereoFx::SIGNAL_INPUT_L));
		addInput(createInput<as_PJ301MPort>(Vec(11, 325), module, SuperDriveStereoFx::SIGNAL_INPUT_R));
		//OUTPUTS
		addOutput(createOutput<as_PJ301MPortGold>(Vec(54, 291), module, SuperDriveStereoFx::SIGNAL_OUTPUT_L));
		addOutput(createOutput<as_PJ301MPortGold>(Vec(54, 325), module, SuperDriveStereoFx::SIGNAL_OUTPUT_R));
	
	}
};

Model *modelSuperDriveStereoFx = createModel<SuperDriveStereoFx, SuperDriveStereoFxWidget>("SuperDriveStereoFx");