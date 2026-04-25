(function() {
    'use strict';

    const controls = {};
    let waveformData = [];
    let meterLevel = 0;

    function init() {
        bindControls();
        setupTransport();
        setupSliders();
        setupWaveform();
        requestInitValues();
    }

    function bindControls() {
        controls.kickBtn = document.getElementById('kick-btn');
        controls.loopBtn = document.getElementById('loop-btn');
        controls.bpm = document.getElementById('bpm');
        controls.waveform = document.getElementById('waveform');
        
        controls.pitch = document.getElementById('pitch');
        controls.pitchVal = document.getElementById('pitch-val');
        controls.pitchDecay = document.getElementById('pitch-decay');
        controls.pitchDecayVal = document.getElementById('pitch-decay-val');
        
        controls.ampAttack = document.getElementById('amp-attack');
        controls.ampAttackVal = document.getElementById('amp-attack-val');
        controls.ampDecay = document.getElementById('amp-decay');
        controls.ampDecayVal = document.getElementById('amp-decay-val');
        controls.ampSustain = document.getElementById('amp-sustain');
        controls.ampSustainVal = document.getElementById('amp-sustain-val');
        controls.ampRelease = document.getElementById('amp-release');
        controls.ampReleaseVal = document.getElementById('amp-release-val');
        
        controls.drive = document.getElementById('drive');
        controls.driveVal = document.getElementById('drive-val');
        controls.color = document.getElementById('color');
        controls.colorVal = document.getElementById('color-val');
        controls.click = document.getElementById('click');
        controls.clickVal = document.getElementById('click-val');
        controls.depth = document.getElementById('depth');
        controls.depthVal = document.getElementById('depth-val');
        
        controls.gain = document.getElementById('gain');
        controls.gainVal = document.getElementById('gain-val');
        
        controls.previewCanvas = document.getElementById('preview-canvas');
        controls.meterFill = document.getElementById('meter-fill');
        controls.meterVal = document.getElementById('meter-val');
    }

    function setupTransport() {
        controls.kickBtn.addEventListener('click', function() {
            sendMessage('trigger', true);
        });

        controls.loopBtn.addEventListener('click', function() {
            this.classList.toggle('active');
            sendMessage('loop', this.classList.contains('active'));
        });

        controls.bpm.addEventListener('change', function() {
            sendMessage('bpm', parseFloat(this.value));
        });

        controls.waveform.addEventListener('change', function() {
            sendMessage('waveform', parseInt(this.value));
        });
    }

    function setupSliders() {
        const sliderConfigs = [
            { slider: controls.pitch, display: controls.pitchVal, format: v => v + ' Hz', param: 'pitch' },
            { slider: controls.pitchDecay, display: controls.pitchDecayVal, format: v => v.toFixed(2), param: 'pitchDecay' },
            { slider: controls.ampAttack, display: controls.ampAttackVal, format: v => v.toFixed(3), param: 'ampAttack' },
            { slider: controls.ampDecay, display: controls.ampDecayVal, format: v => v.toFixed(2), param: 'ampDecay' },
            { slider: controls.ampSustain, display: controls.ampSustainVal, format: v => v.toFixed(2), param: 'ampSustain' },
            { slider: controls.ampRelease, display: controls.ampReleaseVal, format: v => v.toFixed(2), param: 'ampRelease' },
            { slider: controls.drive, display: controls.driveVal, format: v => v.toFixed(2), param: 'drive' },
            { slider: controls.color, display: controls.colorVal, format: v => v.toFixed(2), param: 'color' },
            { slider: controls.click, display: controls.clickVal, format: v => v.toFixed(2), param: 'click' },
            { slider: controls.depth, display: controls.depthVal, format: v => v.toFixed(2), param: 'depth' },
            { slider: controls.gain, display: controls.gainVal, format: v => v.toFixed(2), param: 'gain' }
        ];

        sliderConfigs.forEach(function(config) {
            config.slider.addEventListener('input', function() {
                var value = parseFloat(this.value);
                config.display.textContent = config.format(value);
                sendMessage(config.param, value);
            });
        });
    }

    function setupWaveform() {
        waveformData = new Array(450).fill(0);
        drawWaveform();
        
        setInterval(function() {
            requestMeterLevel();
        }, 50);
    }

    function drawWaveform() {
        var canvas = controls.previewCanvas;
        var ctx = canvas.getContext('2d');
        var width = canvas.width;
        var height = canvas.height;
        var centerY = height / 2;
        
        ctx.fillStyle = '#1a1a1a';
        ctx.fillRect(0, 0, width, height);
        
        ctx.strokeStyle = '#00ffff';
        ctx.lineWidth = 1;
        ctx.beginPath();
        
        for (var i = 0; i < width; i++) {
            var dataIndex = Math.floor(i * waveformData.length / width);
            var sample = waveformData[dataIndex] || 0;
            var y = centerY - sample * centerY * 0.9;
            
            if (i === 0) {
                ctx.moveTo(i, y);
            } else {
                ctx.lineTo(i, y);
            }
        }
        
        ctx.stroke();
        
        ctx.strokeStyle = '#00ffff';
        ctx.globalAlpha = 0.3;
        ctx.beginPath();
        
        for (var i = 0; i < width; i++) {
            var dataIndex = Math.floor(i * waveformData.length / width);
            var sample = waveformData[dataIndex] || 0;
            var y = centerY + sample * centerY * 0.9;
            
            if (i === 0) {
                ctx.moveTo(i, y);
            } else {
                ctx.lineTo(i, y);
            }
        }
        
        ctx.stroke();
        ctx.globalAlpha = 1;
    }

    function sendMessage(param, value) {
        if (typeof juce !== 'undefined' && juce.updateParameter) {
            juce.updateParameter(param, value);
        }
    }

    function requestInitValues() {
        if (typeof juce !== 'undefined' && juce.requestInit) {
            juce.requestInit();
        }
    }

    function requestMeterLevel() {
        if (typeof juce !== 'undefined' && juce.requestMeter) {
            juce.requestMeter();
        }
    }

    window.updateUI = function(data) {
        if (data.waveform !== undefined) {
            controls.waveform.value = data.waveform;
        }
        if (data.pitch !== undefined) {
            controls.pitch.value = data.pitch;
            controls.pitchVal.textContent = data.pitch.toFixed(0) + ' Hz';
        }
        if (data.pitchDecay !== undefined) {
            controls.pitchDecay.value = data.pitchDecay;
            controls.pitchDecayVal.textContent = data.pitchDecay.toFixed(2);
        }
        if (data.ampAttack !== undefined) {
            controls.ampAttack.value = data.ampAttack;
            controls.ampAttackVal.textContent = data.ampAttack.toFixed(3);
        }
        if (data.ampDecay !== undefined) {
            controls.ampDecay.value = data.ampDecay;
            controls.ampDecayVal.textContent = data.ampDecay.toFixed(2);
        }
        if (data.ampSustain !== undefined) {
            controls.ampSustain.value = data.ampSustain;
            controls.ampSustainVal.textContent = data.ampSustain.toFixed(2);
        }
        if (data.ampRelease !== undefined) {
            controls.ampRelease.value = data.ampRelease;
            controls.ampReleaseVal.textContent = data.ampRelease.toFixed(2);
        }
        if (data.drive !== undefined) {
            controls.drive.value = data.drive;
            controls.driveVal.textContent = data.drive.toFixed(2);
        }
        if (data.color !== undefined) {
            controls.color.value = data.color;
            controls.colorVal.textContent = data.color.toFixed(2);
        }
        if (data.click !== undefined) {
            controls.click.value = data.click;
            controls.clickVal.textContent = data.click.toFixed(2);
        }
        if (data.depth !== undefined) {
            controls.depth.value = data.depth;
            controls.depthVal.textContent = data.depth.toFixed(2);
        }
        if (data.gain !== undefined) {
            controls.gain.value = data.gain;
            controls.gainVal.textContent = data.gain.toFixed(2);
        }
        if (data.bpm !== undefined) {
            controls.bpm.value = data.bpm;
        }
        if (data.loopEnabled !== undefined) {
            controls.loopBtn.classList.toggle('active', data.loopEnabled);
        }
    };

    window.updateWaveform = function(data) {
        waveformData = data;
        drawWaveform();
    };

    window.updateMeter = function(db) {
        var meterFill = controls.meterFill;
        var meterLabel = controls.meterVal;
        
        var dbValue = Math.max(-60, db);
        var percentage = ((dbValue + 60) / 60) * 100;
        percentage = Math.max(0, Math.min(100, percentage));
        
        meterFill.style.height = percentage + '%';
        meterLabel.textContent = dbValue > -60 ? dbValue.toFixed(1) + ' dB' : '-∞ dB';
        
        meterFill.classList.remove('red', 'orange', 'yellow');
        if (dbValue >= -0.5) {
            meterFill.classList.add('red');
        } else if (dbValue >= -6) {
            meterFill.classList.add('orange');
        } else if (dbValue >= -12) {
            meterFill.classList.add('yellow');
        }
    };

    document.addEventListener('DOMContentLoaded', init);
})();