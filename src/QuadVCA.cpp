//**************************************************************************************
//Quad QuadVCA module for VCV Rack by Alfredo Santamaria - AS - https://github.com/AScustomWorks/AS
//
//Code adapted from the Fundamentals plugins by Andrew Belt http://www.vcvrack.com
//**************************************************************************************

#include "AS.hpp"

struct QuadVCA : Module {
	enum ParamIds {
		GAIN1_PARAM,
		GAIN2_PARAM,
		GAIN3_PARAM,
		GAIN4_PARAM,
        MODE1_PARAM,
        MODE2_PARAM,
		MODE3_PARAM,
        MODE4_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GAIN1_CV_INPUT,
		IN1_INPUT,
		GAIN2_CV_INPUT,
		IN2_INPUT,
		GAIN3_CV_INPUT,
		IN3_INPUT,
		GAIN4_CV_INPUT,
		IN4_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		OUT3_OUTPUT,
		OUT4_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		GAIN1_LIGHT,
		GAIN2_LIGHT,
		GAIN3_LIGHT,
		GAIN4_LIGHT,
		NUM_LIGHTS
	};

	float v1= 0.0f;
	float v2= 0.0f;
	float v3= 0.0f;
	float v4= 0.0f;
	const float expBase = 50.0f;

	QuadVCA() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(QuadVCA::GAIN1_PARAM, 0.0f, 1.0f, 0.5f, "CH 1 Gain", "%", 0.0f, 100.0f);
		configParam(QuadVCA::GAIN2_PARAM, 0.0f, 1.0f, 0.5f, "CH 2 Gain", "%", 0.0f, 100.0f);
		configParam(QuadVCA::GAIN3_PARAM, 0.0f, 1.0f, 0.5f, "CH 3 Gain", "%", 0.0f, 100.0f);
		configParam(QuadVCA::GAIN4_PARAM, 0.0f, 1.0f, 0.5f, "CH 4 Gain", "%", 0.0f, 100.0f);
		configParam(QuadVCA::MODE1_PARAM, 0.0f, 1.0f, 1.0f, "CH 1 Response");
		configParam(QuadVCA::MODE2_PARAM, 0.0f, 1.0f, 1.0f, "CH 2 Response");
		configParam(QuadVCA::MODE3_PARAM, 0.0f, 1.0f, 1.0f, "CH 3 Response");
		configParam(QuadVCA::MODE4_PARAM, 0.0f, 1.0f, 1.0f, "CH 4 Response");
	}

	void process(const ProcessArgs &args) override {
		//QuadVCA 1
		float out = 0.0;
		v1 = inputs[IN1_INPUT].getVoltage() * params[GAIN1_PARAM].getValue();
		if(inputs[GAIN1_CV_INPUT].isConnected()){
			if(params[MODE1_PARAM].getValue()==1){
				v1 *= clamp(inputs[GAIN1_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
			}else{
				v1 *= rescale(powf(expBase, clamp(inputs[GAIN1_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f)), 1.0f, expBase, 0.0f, 1.0f);
			}
		}
		out+=v1;
		lights[GAIN1_LIGHT].setSmoothBrightness(fmaxf(0.0f, out / 5.0f), args.sampleTime);
		if (outputs[OUT1_OUTPUT].isConnected()) {
				outputs[OUT1_OUTPUT].setVoltage(out);
				out = 0.0f;
		}
		//QuadVCA 2
		v2 = inputs[IN2_INPUT].getVoltage() * params[GAIN2_PARAM].getValue();
		if(inputs[GAIN2_CV_INPUT].isConnected()){
			if(params[MODE2_PARAM].getValue()){
				v2 *= clamp(inputs[GAIN2_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
			}else{
				v2 *= rescale(powf(expBase, clamp(inputs[GAIN2_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f)), 1.0f, expBase, 0.0f, 1.0f);
			}
		}
		out+=v2;
		lights[GAIN2_LIGHT].setSmoothBrightness(fmaxf(0.0f, out / 5.0f), args.sampleTime);
		if (outputs[OUT2_OUTPUT].isConnected()) {
				outputs[OUT2_OUTPUT].setVoltage(out);
				out = 0.0f;
		}
		//QuadVCA 3
		v3 = inputs[IN3_INPUT].getVoltage() * params[GAIN3_PARAM].getValue();
		if(inputs[GAIN3_CV_INPUT].isConnected()){
			if(params[MODE3_PARAM].getValue()){
				v3 *= clamp(inputs[GAIN3_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
			}else{
				v3 *= rescale(powf(expBase, clamp(inputs[GAIN3_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f)), 1.0f, expBase, 0.0f, 1.0f);
			}
		}
		out+=v3;
		lights[GAIN3_LIGHT].setSmoothBrightness(fmaxf(0.0f, out / 5.0f), args.sampleTime);
		if (outputs[OUT3_OUTPUT].isConnected()) {
				outputs[OUT3_OUTPUT].setVoltage(out);
				out = 0.0f;
		}
		//QuadVCA 4
		v4 = inputs[IN4_INPUT].getVoltage() * params[GAIN4_PARAM].getValue();
		if(inputs[GAIN4_CV_INPUT].isConnected()){
			if(params[MODE4_PARAM].getValue()){
				v4 *= clamp(inputs[GAIN4_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
			}else{
				v4 *= rescale(powf(expBase, clamp(inputs[GAIN4_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f)), 1.0f, expBase, 0.0f, 1.0f);
			}
		}
		out+=v4;
		lights[GAIN4_LIGHT].setSmoothBrightness(fmaxf(0.0f, out / 5.0f), args.sampleTime);
		if (outputs[OUT4_OUTPUT].isConnected()) {
				outputs[OUT4_OUTPUT].setVoltage(out);
				out = 0.0f;
		}
	}
};


struct QuadVCAWidget : ModuleWidget { 

	QuadVCAWidget(QuadVCA *module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/QuadVCA.svg")));
		dynamic_cast<SVGPanel*>(panel)->setBorderColor(nvgRGB(0x36, 0x61, 0x7c));
		//SCREWS
		addChild(createWidget<as_HexScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<as_HexScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<as_HexScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<as_HexScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		static const float posX[4] = {11,37,63,89};
		//SLIDERS
		addParam(createParam<as_SlidePot>(Vec(posX[0], 50), module, QuadVCA::GAIN1_PARAM));
		addParam(createParam<as_SlidePot>(Vec(posX[1], 50), module, QuadVCA::GAIN2_PARAM));
		addParam(createParam<as_SlidePot>(Vec(posX[2], 50), module, QuadVCA::GAIN3_PARAM));
		addParam(createParam<as_SlidePot>(Vec(posX[3], 50), module, QuadVCA::GAIN4_PARAM));
		//MODE SWITCHES
		addParam(createParam<as_CKSS>(Vec(posX[0]+1, 183), module, QuadVCA::MODE1_PARAM));
		addParam(createParam<as_CKSS>(Vec(posX[1]+1, 183), module, QuadVCA::MODE2_PARAM));
		addParam(createParam<as_CKSS>(Vec(posX[2]+1, 183), module, QuadVCA::MODE3_PARAM));
		addParam(createParam<as_CKSS>(Vec(posX[3]+1, 183), module, QuadVCA::MODE4_PARAM));
		//CV INPUTS
		addInput(createInput<as_PJ301MPort>(Vec(posX[0]-2, 212), module, QuadVCA::GAIN1_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(posX[1]-2, 212), module, QuadVCA::GAIN2_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(posX[2]-2, 212), module, QuadVCA::GAIN3_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(posX[3]-2, 212), module, QuadVCA::GAIN4_CV_INPUT));
		//INPUTS
		addInput(createInput<as_PJ301MPort>(Vec(posX[0]-2, 261), module, QuadVCA::IN1_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(posX[1]-2, 261), module, QuadVCA::IN2_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(posX[2]-2, 261), module, QuadVCA::IN3_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(posX[3]-2, 261), module, QuadVCA::IN4_INPUT));
		//LEDS
		addChild(createLight<SmallLight<RedLight>>(Vec(posX[0]+7, 289), module, QuadVCA::GAIN1_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(Vec(posX[1]+7, 289), module, QuadVCA::GAIN2_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(Vec(posX[2]+7, 289), module, QuadVCA::GAIN3_LIGHT));
		addChild(createLight<SmallLight<RedLight>>(Vec(posX[3]+7, 289), module, QuadVCA::GAIN4_LIGHT));
		//OUTPUTS
		addOutput(createOutput<as_PJ301MPortGold>(Vec(posX[0]-2, 319), module, QuadVCA::OUT1_OUTPUT));
		addOutput(createOutput<as_PJ301MPortGold>(Vec(posX[1]-2, 319), module, QuadVCA::OUT2_OUTPUT));
		addOutput(createOutput<as_PJ301MPortGold>(Vec(posX[2]-2, 319), module, QuadVCA::OUT3_OUTPUT));
		addOutput(createOutput<as_PJ301MPortGold>(Vec(posX[3]-2, 319), module, QuadVCA::OUT4_OUTPUT));

	}
};


Model *modelQuadVCA = createModel<QuadVCA, QuadVCAWidget>("QuadVCA");