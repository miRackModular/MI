//***********************************************************************************************
//
//Phaser module for VCV Rack by Alfredo Santamaria  - AS - https://github.com/AScustomWorks/AS
//Based on the Phaser Module for VCV Rack by Autodafe http://www.autodafe.net
//Based on code taken from the Fundamentals plugins by Andrew Belt http://www.vcvrack.com
//And part of code on musicdsp.org: http://musicdsp.org/showArchiveComment.php?ArchiveID=78
//
//***********************************************************************************************

#include "AS.hpp"
//#include "dsp/digital.hpp"

#include <stdlib.h>

#define SR api0::engineGetSampleRate()
#define F_PI M_PI

class Phaser{
public:
	Phaser()  //initialise to some useful defaults...
    	: _fb( 0.7f )
    	, _lfoPhase( 0.0f )
    	, _depth( 1.0f )
        , _zm1( 0.f )
    {
    	Range( 440.0f, 1600.0f );
    	Rate( 0.5f );
    }

    void Range( float fMin, float fMax ){ // Hz
    	_dmin = fMin / (SR/2.0f);
        _dmax = fMax / (SR/2.0f);
    }

    void Rate( float rate ){ // cps
    	_lfoInc = 2.0f * F_PI * (rate / SR);
    }

    void Feedback( float fb ){ // 0 -> <1.
    	_fb = fb;
    }

    void Depth( float depth ){  // 0 -> 1.
     	_depth = depth;
    }

    float Update( float inSamp ){
    	//calculate and update phaser sweep lfo...
        float d  = _dmin + (_dmax-_dmin) * ((sin( _lfoPhase ) + 1.f)/2.f);
        _lfoPhase += _lfoInc;
        if( _lfoPhase >= F_PI * 2.0f )
        	_lfoPhase -= F_PI * 2.0f;

        //update filter coeffs
        for( int i=0; i<6; i++ )
        	_alps[i].Delay( d );

        //calculate output
        float y = 	_alps[0].Update(
        			 _alps[1].Update(
                      _alps[2].Update(
                       _alps[3].Update(
                        _alps[4].Update(
                         _alps[5].Update( inSamp + _zm1 * _fb ))))));
        _zm1 = y;

        return inSamp + y * _depth;
    }
private:
	class AllpassDelay{
    public:
    	AllpassDelay()
        	: _a1( 0.0f )
            , _zm1( 0.0f )
            {}

        void Delay( float delay ){ //sample delay time
        	_a1 = (1.0f - delay) / (1.0f + delay);
        }

        float Update( float inSamp ){
        	float y = inSamp * -_a1 + _zm1;
        	_zm1 = y * _a1 + inSamp;

            return y;
        }
    private:
    	float _a1, _zm1;
    };

    AllpassDelay _alps[6];

    float _dmin, _dmax; //range
    float _fb; //feedback
    float _lfoPhase;
    float _lfoInc;
    float _depth;

    float _zm1;
};

struct PhaserFx : Module{
	enum ParamIds {
		RATE_PARAM,
		FBK_PARAM,
		DEPTH_PARAM,
        BYPASS_SWITCH,
		NUM_PARAMS
	};
	enum InputIds {
		INPUT,
		RATE_CV_INPUT,
		FEEDBACK_CV_INPUT,
		DEPTH_CV_INPUT,
		BYPASS_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT,
		NUM_OUTPUTS
	};
	  enum LightIds {
		RATE_LIGHT,
		FBK_LIGHT,
		DEPTH_LIGHT,
		BYPASS_LED,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger bypass_button_trig;
	dsp::SchmittTrigger bypass_cv_trig;
	Phaser pha;

	bool fx_bypass = false;

	float fade_in_fx = 0.0f;
	float fade_in_dry = 0.0f;
	float fade_out_fx = 1.0f;
	float fade_out_dry = 1.0f;
    const float fade_speed = 0.001f;

	PhaserFx() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PhaserFx::RATE_PARAM, 0.0f, 1.0f, 0.0f, "Rate", "%", 0.0f, 100.0f);
		configParam(PhaserFx::FBK_PARAM, 0.0f, 0.95f, 0.0f, "Feedback", "%", 0.0f, 100.0f);
		configParam(PhaserFx::DEPTH_PARAM, 0.0f, 1.0f, 0.0f, "Depth", "%", 0.0f, 100.0f);
		configParam(PhaserFx::BYPASS_SWITCH , 0.0f, 1.0f, 0.0f, "Bypass");	
	}

	void process(const ProcessArgs &args) override;
    
    void onSampleRateChange() override {
		pha.Range( 440.0f, 1600.0f );	
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

	void resetFades(){
		fade_in_fx = 0.0f;
		fade_in_dry = 0.0f;
		fade_out_fx = 1.0f;
		fade_out_dry = 1.0f;
	}

};

void PhaserFx::process(const ProcessArgs &args) {

	if (bypass_button_trig.process(params[BYPASS_SWITCH].getValue())	|| bypass_cv_trig.process(inputs[BYPASS_CV_INPUT].getVoltage()) ){
			fx_bypass = !fx_bypass;
			resetFades();
	}

    lights[BYPASS_LED].value = fx_bypass ? 1.00 : 0.0;


	float rate = clamp(params[RATE_PARAM].getValue() + inputs[RATE_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
	float feedback = clamp(params[FBK_PARAM].getValue() + inputs[FEEDBACK_CV_INPUT].getVoltage() / 10.0f, 0.0f, 0.95f);
	float depth = clamp(params[DEPTH_PARAM].getValue() + inputs[DEPTH_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);

	float input = inputs[INPUT].getVoltage() / 5.0f;

		//TODO: only if changed
		pha.Rate(rate);
		pha.Feedback(feedback);
		pha.Depth (depth);
	
	float out = pha.Update(input);

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
        outputs[OUT].setVoltage(( (input * 5.0f) * fade_in_dry ) + ( (out*5.0f) * fade_out_fx ));
    }else{
		fade_in_fx += fade_speed;
		if ( fade_in_fx > 1.0f ) {
			fade_in_fx = 1.0f;
		}
		fade_out_dry -= fade_speed;
		if ( fade_out_dry < 0.0f ) {
			fade_out_dry = 0.0f;
		}
        outputs[OUT].setVoltage(( (input * 5.0f) * fade_out_dry ) + ( (out*5.0f) * fade_in_fx ));
	}

	lights[RATE_LIGHT].value = clamp(params[RATE_PARAM].getValue() + inputs[RATE_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
	lights[FBK_LIGHT].value = clamp(params[FBK_PARAM].getValue() + inputs[FEEDBACK_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
	lights[DEPTH_LIGHT].value = clamp(params[DEPTH_PARAM].getValue() + inputs[DEPTH_CV_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
}


struct PhaserFxWidget : ModuleWidget { 

	PhaserFxWidget(PhaserFx *module) {
		
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Phaser.svg")));
		dynamic_cast<SVGPanel*>(panel)->setBorderColor(nvgRGB(0x36, 0x61, 0x7c));
	
		//SCREWS
		addChild(createWidget<as_HexScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<as_HexScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<as_HexScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<as_HexScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		//KNOBS  
		addParam(createParam<as_KnobSmall>(Vec(8, 50), module, PhaserFx::RATE_PARAM));
		addParam(createParam<as_KnobSmall>(Vec(51, 50), module, PhaserFx::FBK_PARAM));
		addParam(createParam<as_KnobSmall>(Vec(8, 145), module, PhaserFx::DEPTH_PARAM));
		//LIGHTS
		addChild(createLight<SmallLight<YellowLight>>(Vec(30, 81.5), module, PhaserFx::RATE_LIGHT));
		addChild(createLight<SmallLight<YellowLight>>(Vec(73, 81.5), module, PhaserFx::FBK_LIGHT));
		addChild(createLight<SmallLight<YellowLight>>(Vec(30, 176.5), module, PhaserFx::DEPTH_LIGHT));
		//BYPASS SWITCH
		addParam(createParam<LEDBezel>(Vec(55, 149), module, PhaserFx::BYPASS_SWITCH ));
		addChild(createLight<LedLight<RedLight>>(Vec(55+2.2, 149+2), module, PhaserFx::BYPASS_LED));
		//INS/OUTS
		addInput(createInput<as_PJ301MPort>(Vec(11, 291), module, PhaserFx::INPUT));
		addOutput(createOutput<as_PJ301MPortGold>(Vec(54, 291), module, PhaserFx::OUT));
		//CV INPUTS
		addInput(createInput<as_PJ301MPort>(Vec(11, 89), module, PhaserFx::RATE_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(54, 89), module, PhaserFx::FEEDBACK_CV_INPUT));
		addInput(createInput<as_PJ301MPort>(Vec(11, 184), module, PhaserFx::DEPTH_CV_INPUT));

		//BYPASS CV INPUT
		addInput(createInput<as_PJ301MPort>(Vec(54, 184), module, PhaserFx::BYPASS_CV_INPUT));
	
	}
};


Model *modelPhaserFx = createModel<PhaserFx, PhaserFxWidget>("PhaserFx");