(function() {
    'use strict';

    const controls = {};
    let waveformData = [];

    function init() {
        bindControls();
        setupTransport();
        setupSliders();
        setupAbout();
        setupPresets();
        setupWaveform();
        setupEventListeners();
        notifyReady();
    }

    function bindControls() {
        controls.kickBtn = document.getElementById('kick-btn');
        controls.loopBtn = document.getElementById('loop-btn');
        controls.bpm = document.getElementById('bpm');
        controls.waveform = document.getElementById('waveform');

        controls.previewCanvas = document.getElementById('preview-canvas');
        controls.meterFill = document.getElementById('meter-fill');
        controls.meterVal = document.getElementById('meter-val');

        controls.aboutBtn = document.getElementById('about-btn');
        controls.aboutOverlay = document.getElementById('about-overlay');
        controls.closeAbout = document.getElementById('close-about');
        controls.websiteLink = document.getElementById('website-link');

        controls.presetSelect = document.getElementById('preset-select');
        controls.saveBtn = document.getElementById('save-btn');
        controls.loadBtn = document.getElementById('load-btn');
    }

    function setupAbout() {
        controls.aboutBtn.addEventListener('click', () => controls.aboutOverlay.classList.remove('hidden'));
        controls.closeAbout.addEventListener('click', () => controls.aboutOverlay.classList.add('hidden'));
        controls.aboutOverlay.addEventListener('click', (e) => {
            if (e.target === controls.aboutOverlay) controls.aboutOverlay.classList.add('hidden');
        });

        controls.websiteLink.addEventListener('click', (e) => {
            e.preventDefault();
            emitEvent('openURL', 'https://www.sanusart.com');
        });
    }

    function setupPresets() {
        controls.presetSelect.addEventListener('change', function() {
            const raw = this.value;
            if (raw === '' || raw === '-1') {
                return;
            }
            const val = parseInt(raw, 10);
            if (!Number.isNaN(val) && val >= 0) {
                emitEvent('parameterChange', { param: 'preset', value: val });
            }
        });

        controls.saveBtn.addEventListener('click', () => emitEvent('savePreset', null));
        controls.loadBtn.addEventListener('click', () => emitEvent('loadPreset', null));
    }

    function setupTransport() {
        controls.kickBtn.addEventListener('mousedown', function() {
            emitEvent('trigger', 1);
        });

        controls.loopBtn.addEventListener('click', function() {
            const active = !this.classList.contains('active');
            emitEvent('parameterChange', { param: 'loopEnabled', value: active ? 1 : 0 });
        });

        controls.bpm.addEventListener('change', function() {
            emitEvent('parameterChange', { param: 'bpm', value: parseFloat(this.value) });
        });

        const waveOptions = controls.waveform.querySelectorAll('.wave-option');
        waveOptions.forEach(function(opt) {
            opt.addEventListener('click', function() {
                waveOptions.forEach(function(o) {
                    o.classList.remove('active');
                    o.setAttribute('aria-checked', 'false');
                });
                this.classList.add('active');
                this.setAttribute('aria-checked', 'true');
                emitEvent('parameterChange', { param: 'waveform', value: parseInt(this.dataset.val, 10) });
            });
        });
    }

    function setupEventListeners() {
        if (window.__JUCE__ && window.__JUCE__.backend) {
            window.__JUCE__.backend.addEventListener('initUI', function(data) {
                window.updateUI(data);
            });
            window.__JUCE__.backend.addEventListener('meterUpdate', function(data) {
                window.updateMeter(data);
            });
            window.__JUCE__.backend.addEventListener('waveformUpdate', function(data) {
                window.updateWaveform(data);
            });
            window.__JUCE__.backend.addEventListener('blinkKick', function() {
                window.blinkKick();
            });
        } else {
            console.warn('JUCE backend not available');
        }
    }

    function notifyReady() {
        if (window.__JUCE__ && window.__JUCE__.backend) {
            window.__JUCE__.backend.emitEvent('uiReady', null);
        }
    }

    function emitEvent(type, data) {
        if (window.__JUCE__ && window.__JUCE__.backend) {
            window.__JUCE__.backend.emitEvent(type, data);
        } else {
            console.warn('JUCE backend emitEvent not available for:', type);
        }
    }

    const activeKnobs = {};

    function formatAdsrTime(seconds) {
        var s = Math.max(0, seconds);
        if (s < 1) {
            var ms = s * 1000;
            if (ms < 100) {
                return ms.toFixed(1) + ' ms';
            }
            return Math.round(ms) + ' ms';
        }
        return s.toFixed(2) + ' s';
    }

    function formatSustainPct(v) {
        return Math.round(Math.max(0, Math.min(1, v)) * 100) + '%';
    }

    function refreshEnvelopePreview() {
        var elA = document.getElementById('slider-amp-attack');
        var elD = document.getElementById('slider-amp-decay');
        var elS = document.getElementById('slider-amp-sustain');
        var elR = document.getElementById('slider-amp-release');
        var fillPath = document.getElementById('adsr-fill-path');
        var strokePath = document.getElementById('adsr-stroke-path');
        if (!elA || !fillPath || !strokePath) {
            return;
        }

        var a = parseFloat(elA.value);
        var d = parseFloat(elD.value);
        var sus = parseFloat(elS.value);
        var r = parseFloat(elR.value);

        var x0 = 8;
        var xMax = 192;
        var usable = xMax - x0;
        var baseline = 35;
        var yPeak = 6;
        var ampRange = baseline - yPeak;

        var ta = Math.max(a, 1e-6);
        var td = Math.max(d, 0.05);
        var tr = Math.max(r, 0.05);
        var sum = ta + td + tr;

        var wA = (ta / sum) * usable;
        var wD = (td / sum) * usable;
        var wR = (tr / sum) * usable;
        var wP = usable - wA - wD - wR;
        var minPlateau = 16;
        if (wP < minPlateau) {
            var cap = usable - minPlateau;
            var sumW = wA + wD + wR;
            if (sumW > 1e-6) {
                var sc = cap / sumW;
                wA *= sc;
                wD *= sc;
                wR *= sc;
            }
            wP = minPlateau;
        }

        var x1 = x0 + wA;
        var x2 = x1 + wD;
        var x3 = x2 + wP;
        var x4 = Math.min(x3 + wR, xMax);

        var ys = baseline - sus * ampRange;

        function band(id, x, w) {
            var el = document.getElementById(id);
            if (!el) return;
            var ww = Math.max(0.35, w);
            el.setAttribute('x', String(x));
            el.setAttribute('width', String(ww));
        }

        band('adsr-bg-a', x0, x1 - x0);
        band('adsr-bg-d', x1, x2 - x1);
        band('adsr-bg-s', x2, x3 - x2);
        band('adsr-bg-r', x3, x4 - x3);

        var cA1 = x0 + (x1 - x0) * 0.38;
        var cA2 = x1 - (x1 - x0) * 0.06;
        var cD1 = x1 + (x2 - x1) * 0.14;
        var cD2 = x2 - (x2 - x1) * 0.32;
        var cR1 = x3 + (x4 - x3) * 0.22;
        var cR2 = x4 - (x4 - x3) * 0.1;

        var dOpen = 'M ' + x0 + ',' + baseline
            + ' C ' + cA1 + ',' + baseline + ' ' + cA2 + ',' + yPeak + ' ' + x1 + ',' + yPeak
            + ' C ' + cD1 + ',' + yPeak + ' ' + cD2 + ',' + ys + ' ' + x2 + ',' + ys
            + ' L ' + x3 + ',' + ys
            + ' C ' + cR1 + ',' + ys + ' ' + cR2 + ',' + baseline + ' ' + x4 + ',' + baseline;

        strokePath.setAttribute('d', dOpen);
        fillPath.setAttribute('d', dOpen + ' L ' + x0 + ',' + baseline + ' Z');

        var susLine = document.getElementById('adsr-sustain-line');
        if (susLine) {
            if (sus > 0.02 && x3 > x2 + 0.5) {
                susLine.setAttribute('x1', String(x2));
                susLine.setAttribute('x2', String(x3));
                susLine.setAttribute('y1', String(ys));
                susLine.setAttribute('y2', String(ys));
                susLine.setAttribute('opacity', '0.4');
            } else {
                susLine.setAttribute('opacity', '0');
            }
        }

        var yMid = baseline - ampRange * 0.5;
        var gridMid = document.querySelector('.adsr-grid-mid');
        if (gridMid) {
            gridMid.setAttribute('y1', String(yMid));
            gridMid.setAttribute('y2', String(yMid));
        }

        function setVLine(id, x) {
            var ln = document.getElementById(id);
            if (!ln) return;
            ln.setAttribute('x1', String(x));
            ln.setAttribute('x2', String(x));
            ln.setAttribute('y1', String(yPeak));
            ln.setAttribute('y2', String(baseline));
        }

        setVLine('adsr-edge-a', x1);
        setVLine('adsr-edge-d', x2);
        setVLine('adsr-edge-s', x3);
        setVLine('adsr-edge-r', x4);
    }

    function formatRootReadout(hz) {
        var h = Math.round(Math.max(40, Math.min(250, Number(hz))));
        var midi = Math.round(69 + 12 * (Math.log(h / 440) / Math.LN2));
        midi = Math.max(12, Math.min(83, midi));
        var names = ['C', 'C♯', 'D', 'D♯', 'E', 'F', 'F♯', 'G', 'G♯', 'A', 'A♯', 'B'];
        var name = names[midi % 12] + (Math.floor(midi / 12) - 1);
        return name + ' · ' + h + ' Hz';
    }

    function setupKeyNoteControls() {
        var rail = document.querySelector('.key-note-rail');
        var octDown = document.getElementById('key-oct-down');
        var octUp = document.getElementById('key-oct-up');
        var octDisplay = document.getElementById('key-oct-display');
        var readout = document.getElementById('key-tuner-readout');
        if (!rail || !octDown || !octUp || !octDisplay) {
            return;
        }

        function setOctLabel(val) {
            var v = Math.max(0, Math.min(5, Math.round(Number(val))));
            octDisplay.dataset.value = String(v);
            octDisplay.textContent = v === 0 ? '0 · sub' : String(v);
            octDown.disabled = v <= 0;
            octUp.disabled = v >= 5;
        }

        function setActiveNote(val) {
            var n = Math.max(0, Math.min(11, Math.round(Number(val))));
            rail.querySelectorAll('.key-note-btn').forEach(function(btn) {
                var on = parseInt(btn.dataset.note, 10) === n;
                btn.classList.toggle('key-note-btn--active', on);
                btn.setAttribute('aria-checked', on ? 'true' : 'false');
            });
        }

        activeKnobs.keyNote = function(val) {
            setActiveNote(val);
        };
        activeKnobs.keyOctave = function(val) {
            setOctLabel(val);
        };

        rail.querySelectorAll('.key-note-btn').forEach(function(btn) {
            btn.setAttribute('role', 'radio');
            btn.addEventListener('click', function() {
                emitEvent('parameterChange', { param: 'keyNote', value: parseInt(btn.dataset.note, 10) });
            });
        });

        octDown.addEventListener('click', function() {
            var cur = parseInt(octDisplay.dataset.value || '3', 10);
            emitEvent('parameterChange', { param: 'keyOctave', value: Math.max(0, cur - 1) });
        });
        octUp.addEventListener('click', function() {
            var cur = parseInt(octDisplay.dataset.value || '3', 10);
            emitEvent('parameterChange', { param: 'keyOctave', value: Math.min(5, cur + 1) });
        });

        if (readout) {
            readout.textContent = formatRootReadout(196);
        }
    }

    function setupParamSlider(sliderId, param, formatFn, afterChange) {
        const input = document.getElementById(sliderId);
        if (!input) return;

        const dispId = sliderId.replace(/^slider-/, 'val-').replace(/slider-amp-/, 'val-amp-');
        const disp = document.getElementById(dispId);
        const min = parseFloat(input.min);
        const max = parseFloat(input.max);

        const clamp = (v) => Math.max(min, Math.min(max, v));

        const syncFromValue = (val) => {
            const v = clamp(typeof val === 'number' ? val : parseFloat(val));
            input.value = String(v);
            if (disp) disp.textContent = formatFn(v);
            if (afterChange) afterChange();
        };

        input.addEventListener('input', () => {
            const v = parseFloat(input.value);
            if (disp) disp.textContent = formatFn(v);
            emitEvent('parameterChange', { param: param, value: v });
            if (afterChange) afterChange();
        });

        activeKnobs[param] = syncFromValue;
    }

    function formatHpHz(v) {
        var n = Math.round(Number(v));
        return n + ' Hz';
    }

    function formatLpHz(v) {
        var x = Math.max(800, Math.min(20000, Number(v)));
        if (x >= 1000) {
            var k = x / 1000;
            var s = (Math.abs(k - Math.round(k)) < 0.06) ? String(Math.round(k)) : k.toFixed(1);
            return s + ' kHz';
        }
        return Math.round(x) + ' Hz';
    }

    function setupSliders() {
        setupParamSlider('slider-pitch', 'pitch', (v) => Math.round(v) + ' Hz');
        setupParamSlider('slider-pitch-decay', 'pitchDecay', (v) => v.toFixed(2));
        setupParamSlider('slider-amp-attack', 'ampAttack', formatAdsrTime, refreshEnvelopePreview);
        setupParamSlider('slider-amp-decay', 'ampDecay', formatAdsrTime, refreshEnvelopePreview);
        setupParamSlider('slider-amp-sustain', 'ampSustain', formatSustainPct, refreshEnvelopePreview);
        setupParamSlider('slider-amp-release', 'ampRelease', formatAdsrTime, refreshEnvelopePreview);
        setupParamSlider('slider-click', 'click', (v) => v.toFixed(2));
        setupParamSlider('slider-click-pitch', 'clickPitch', (v) => String(Math.round(v)));

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
                if (dial) dial.style.transform = 'rotate(' + angle + 'deg)';
                if (display) display.textContent = formatFn(currentVal);
            };

            let isDragging = false;
            let startY = 0;
            let startVal = 0;

            container.addEventListener('mousedown', (e) => {
                e.preventDefault();
                e.stopPropagation();
                isDragging = true;
                startY = e.clientY;
                startVal = currentVal;
                document.body.style.cursor = 'ns-resize';
                document.body.style.overflow = 'hidden';
            });

            window.addEventListener('mousemove', (e) => {
                if (!isDragging) return;
                e.preventDefault();
                e.stopPropagation();
                const deltaY = startY - e.clientY;
                const deltaVal = (deltaY / 120) * (max - min);
                updateVisuals(startVal + deltaVal);
                emitEvent('parameterChange', { param: param, value: currentVal });
            }, { passive: false });

            window.addEventListener('mouseup', (e) => {
                if (isDragging) {
                    isDragging = false;
                    document.body.style.cursor = 'default';
                    document.body.style.overflow = '';
                }
            });

            activeKnobs[param] = updateVisuals;
        }

        setupKnob('knob-drive', 0.5, 3, 'drive', (v) => v.toFixed(2));
        setupKnob('knob-color', 0, 1, 'color', (v) => v.toFixed(2));
        setupKnob('knob-wave-morph', 0, 1, 'waveMorph', (v) => v.toFixed(2));
        setupKnob('knob-depth', 0, 1, 'depth', (v) => v.toFixed(2));
        setupKnob('knob-gain', 0, 1.5, 'gain', (v) => v.toFixed(2));
        setupKnob('knob-enhancer', 0, 1, 'enhancer', (v) => v.toFixed(2));
        setupKnob('knob-hp', 25, 600, 'hpCutoff', formatHpHz);
        setupKnob('knob-lp', 800, 20000, 'lpCutoff', formatLpHz);

        setupKeyNoteControls();

        refreshEnvelopePreview();
    }

    function setupWaveform() {
        waveformData = new Array(450).fill(0);
        drawWaveform();
    }

    function drawWaveform() {
        var canvas = controls.previewCanvas;
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        var width = canvas.width;
        var height = canvas.height;
        var centerY = height / 2;

        ctx.fillStyle = '#141821';
        ctx.fillRect(0, 0, width, height);

        ctx.strokeStyle = '#5bdbee';
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

        ctx.strokeStyle = '#5bdbee';
        ctx.globalAlpha = 0.28;
        ctx.beginPath();

        for (var j = 0; j < width; j++) {
            var di = Math.floor(j * waveformData.length / width);
            var s = waveformData[di] || 0;
            var y2 = centerY + s * centerY * 0.9;

            if (j === 0) {
                ctx.moveTo(j, y2);
            } else {
                ctx.lineTo(j, y2);
            }
        }

        ctx.stroke();
        ctx.globalAlpha = 1;
    }

    window.juce = window.juce || {};

    var presetPlaceholderLabel = 'Choose preset…';

    window.updateUI = function(data) {
        if (controls.presetSelect && (data.factoryPreset !== undefined || data.presetDisplayName !== undefined)) {
            var ph = controls.presetSelect.querySelector('option[value=""]');
            var fp = data.factoryPreset;
            var displayName = (typeof data.presetDisplayName === 'string' && data.presetDisplayName.length > 0)
                ? data.presetDisplayName
                : 'Default';
            var presetValStr = (typeof fp === 'number' && fp >= 0 && Number.isFinite(fp))
                ? String(Math.floor(fp))
                : '';
            var presetOpt = presetValStr ? controls.presetSelect.querySelector('option[value="' + presetValStr + '"]') : null;
            if (presetOpt) {
                controls.presetSelect.value = presetValStr;
                if (ph) {
                    ph.textContent = presetPlaceholderLabel;
                }
                var selOpt = controls.presetSelect.options[controls.presetSelect.selectedIndex];
                var shown = selOpt ? selOpt.textContent.replace(/\s+/g, ' ').trim() : displayName;
                controls.presetSelect.setAttribute('aria-label', 'Preset: ' + shown);
            } else {
                controls.presetSelect.value = '';
                var truncated = displayName.length > 42 ? displayName.slice(0, 39) + '…' : displayName;
                if (ph) {
                    ph.textContent = truncated;
                }
                controls.presetSelect.setAttribute('aria-label', 'Preset: ' + displayName);
            }
        }

        if (data.pitch !== undefined) {
            var kr = document.getElementById('key-tuner-readout');
            if (kr) {
                kr.textContent = formatRootReadout(data.pitch);
            }
        }

        if (data.waveform !== undefined && controls.waveform) {
            const waveOptions = controls.waveform.querySelectorAll('.wave-option');
            waveOptions.forEach(function(o) {
                const on = parseInt(o.dataset.val, 10) === data.waveform;
                o.classList.toggle('active', on);
                o.setAttribute('aria-checked', on ? 'true' : 'false');
            });
        }

        Object.keys(activeKnobs).forEach(function(param) {
            if (data[param] !== undefined) {
                activeKnobs[param](data[param]);
            }
        });

        if (data.bpm !== undefined && controls.bpm) {
            controls.bpm.value = data.bpm;
        }
        if (data.loopEnabled !== undefined && controls.loopBtn) {
            controls.loopBtn.classList.toggle('active', data.loopEnabled);
            controls.loopBtn.setAttribute('aria-pressed', data.loopEnabled ? 'true' : 'false');
        }
        if (data.isStandalone !== undefined) {
            document.body.classList.toggle('is-standalone', !!data.isStandalone);
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

        if (!meterFill || !meterLabel) return;

        var dbValue = Math.max(-60, db);
        var percentage = ((dbValue + 60) / 60) * 100;
        percentage = Math.max(0, Math.min(100, percentage));

        meterFill.style.width = percentage + '%';

        var text = dbValue > -60 ? dbValue.toFixed(1) : '\u2212\u221e';
        meterLabel.textContent = text + ' dB';

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
