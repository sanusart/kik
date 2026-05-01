(function() {
    'use strict';

    const controls = {};
    let waveformData = [];
    let meterLevel = 0;

    function init() {
        console.log('init called');
        bindControls();
        console.log('bindControls done');
        setupTransport();
        console.log('setupTransport done');
        setupSliders();
        console.log('setupSliders done');
        setupWaveform();
        console.log('setupWaveform done');
        requestInitValues();
        console.log('requestInitValues done');
    }

    function bindControls() {
        controls.kickBtn = document.getElementById('kick-btn');
        controls.loopBtn = document.getElementById('loop-btn');
        controls.bpm = document.getElementById('bpm');
        controls.waveform = document.getElementById('waveform');
        
        controls.previewCanvas = document.getElementById('preview-canvas');
        controls.meterFill = document.getElementById('meter-fill');
        controls.meterVal = document.getElementById('meter-val');
    }

    function setupTransport() {
        controls.kickBtn.addEventListener('mousedown', function() {
            sendMessage('trigger', true);
        });

        controls.loopBtn.addEventListener('click', function() {
            this.classList.toggle('active');
            sendMessage('loop', this.classList.contains('active') ? 1 : 0);
        });

        controls.bpm.addEventListener('change', function() {
            sendMessage('bpm', parseFloat(this.value));
        });

        const waveOptions = controls.waveform.querySelectorAll('.wave-option');
        waveOptions.forEach(function(opt) {
            opt.addEventListener('click', function() {
                waveOptions.forEach(function(o) { o.classList.remove('active'); });
                this.classList.add('active');
                sendMessage('waveform', parseInt(this.dataset.val));
            });
        });
    }

    const activeKnobs = {};

    function setupSliders() {
        function setupKnob(id, min, max, param, formatFn) {
            const container = document.getElementById(id);
            if (!container) return;
            const dial = container.querySelector('.knob-dial');
            const displayId = 'val-' + id.replace('knob-', '');
            const display = document.getElementById(displayId);
            let currentVal = min;

            const updateVisuals = (val) => {
                currentVal = Math.max(min, Math.min(max, val));
                const percentage = (currentVal - min) / (max - min);
                const angle = -140 + (percentage * 280); 
                if (dial) dial.style.transform = `rotate(${angle}deg)`;
                if (display) display.textContent = formatFn(currentVal);
            };

            let isDragging = false;
            let startY = 0;
            let startVal = 0;

            container.addEventListener('mousedown', (e) => {
                isDragging = true;
                startY = e.clientY;
                startVal = currentVal;
                document.body.style.cursor = 'ns-resize';
            });

            window.addEventListener('mousemove', (e) => {
                if (!isDragging) return;
                const deltaY = startY - e.clientY;
                const deltaVal = (deltaY / 120) * (max - min); 
                updateVisuals(startVal + deltaVal);
                sendMessage(param, currentVal);
            });

            window.addEventListener('mouseup', () => {
                if (isDragging) {
                    isDragging = false;
                    document.body.style.cursor = 'default';
                }
            });

            activeKnobs[param] = updateVisuals;
        }

        setupKnob('knob-pitch', 40, 250, 'pitch', v => Math.round(v).toString() + ' Hz');
        setupKnob('knob-pitch-decay', 0.05, 0.5, 'pitchDecay', v => v.toFixed(2));
        setupKnob('knob-amp-attack', 0, 0.05, 'ampAttack', v => v.toFixed(3));
        setupKnob('knob-amp-decay', 0.05, 1, 'ampDecay', v => v.toFixed(2));
        setupKnob('knob-amp-sustain', 0, 1, 'ampSustain', v => v.toFixed(2));
        setupKnob('knob-amp-release', 0.05, 1, 'ampRelease', v => v.toFixed(2));
        setupKnob('knob-drive', 0.5, 3, 'drive', v => v.toFixed(2));
        setupKnob('knob-color', 0, 1, 'color', v => v.toFixed(2));
        setupKnob('knob-click', 0, 1, 'click', v => v.toFixed(2));
        setupKnob('knob-click-pitch', 500, 8000, 'clickPitch', v => Math.round(v).toString());
        setupKnob('knob-depth', 0, 1, 'depth', v => v.toFixed(2));
        setupKnob('knob-gain', 0, 1.5, 'gain', v => v.toFixed(2));
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

    window.juce = window.juce || {};

    function sendMessage(param, value) {
        console.log('sendMessage: ' + param + '=' + value);
        var sent = false;
        if (typeof window.__JUCE__ !== 'undefined' && window.__JUCE__.getNativeFunction) {
            try { 
                window.__JUCE__.getNativeFunction('JUCE_CALLBACK')(param, value); 
                sent = true; 
            } catch(e) {
                console.error(e);
            }
        }
        if (!sent && typeof window.Juce !== 'undefined' && window.Juce.getNativeFunction) {
            try { 
                window.Juce.getNativeFunction('JUCE_CALLBACK')(param, value); 
                sent = true; 
            } catch(e) {
                console.error(e);
            }
        }
        if (!sent) {
            window.__paramQueue__ = window.__paramQueue__ || [];
            window.__paramQueue__.push({param: param, value: value});
        }
    }

    function requestInitValues() {
        sendMessage('__init__', 0);
    }

    function requestMeterLevel() {
        // C++ backend pushes meter automatically, no need to request manually
    }

    window.updateUI = function(data) {
        if (data.waveform !== undefined) {
            const waveOptions = controls.waveform.querySelectorAll('.wave-option');
            waveOptions.forEach(function(o) {
                if (parseInt(o.dataset.val) === data.waveform) {
                    o.classList.add('active');
                } else {
                    o.classList.remove('active');
                }
            });
        }
        Object.keys(activeKnobs).forEach(param => {
            if (data[param] !== undefined) {
                activeKnobs[param](data[param]);
            }
        });

        if (data.bpm !== undefined) {
            controls.bpm.value = data.bpm;
        }
        if (data.loopEnabled !== undefined) {
            controls.loopBtn.classList.toggle('active', data.loopEnabled);
        }
        if (data.isStandalone !== undefined) {
            if (data.isStandalone) {
                document.body.classList.add('is-standalone');
            } else {
                document.body.classList.remove('is-standalone');
            }
        }
    };

    window.blinkKick = function() {
        if (!controls.kickBtn) return;
        controls.kickBtn.classList.add('midi-active');
        setTimeout(() => {
            controls.kickBtn.classList.remove('midi-active');
        }, 120);
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
        
        var text = dbValue > -60 ? dbValue.toFixed(1) : '-INF';
        meterLabel.textContent = text.padStart(5, ' ') + ' dB';
        
        meterFill.classList.remove('red', 'orange', 'yellow');
        if (dbValue >= -0.5) {
            meterFill.classList.add('red');
        } else if (dbValue >= -6) {
            meterFill.classList.add('orange');
        } else if (dbValue >= -12) {
            meterFill.classList.add('yellow');
        }
    };

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();