//
//  VirtualPadView.swift
//  app-swift
//
//  Created by Yoji Suzuki on 2023/07/01.
//

import UIKit
import MSX2

class VirtualPadView: UIView {
    let joyPad = MSX2JoyPad()
    let cursorContainer = UIView()
    let cursorUp = UIImageView(image: UIImage(named: "pad_up_off"))
    let cursorDown = UIImageView(image: UIImage(named: "pad_down_off"))
    let cursorLeft = UIImageView(image: UIImage(named: "pad_left_off"))
    let cursorRight = UIImageView(image: UIImage(named: "pad_right_off"))
    let buttonContainer = UIView()
    let labelA = UILabel()
    let labelB = UILabel()
    let buttonA = UIImageView(image: UIImage(named: "pad_btn_off"))
    let buttonB = UIImageView(image: UIImage(named: "pad_btn_off"))
    let ctrlContainer = UIView()
    let labelSelect = UILabel()
    let labelStart = UILabel()
    let ctrlSelect = UIImageView(image: UIImage(named: "pad_ctrl_off"))
    let ctrlStart = UIImageView(image: UIImage(named: "pad_ctrl_off"))
    var cursorTouches = Array<UITouch>()
    var buttonTouches = Array<UITouch>()
    var ctrlTouches = Array<UITouch>()
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setup()
    }
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }
    
    private func setup() {
        cursorContainer.addSubview(cursorUp)
        cursorContainer.addSubview(cursorDown)
        cursorContainer.addSubview(cursorLeft)
        cursorContainer.addSubview(cursorRight)
        addSubview(cursorContainer)
        buttonContainer.addSubview(labelA)
        buttonContainer.addSubview(labelB)
        buttonContainer.addSubview(buttonA)
        buttonContainer.addSubview(buttonB)
        addSubview(buttonContainer)
        ctrlContainer.addSubview(labelStart)
        ctrlContainer.addSubview(labelSelect)
        ctrlContainer.addSubview(ctrlStart)
        ctrlContainer.addSubview(ctrlSelect)
        addSubview(ctrlContainer)
        setupLabel(label: labelA, text: "A")
        setupLabel(label: labelB, text: "B")
        setupLabel(label: labelStart, text: "SPACE")
        setupLabel(label: labelSelect, text: "ESC")
        cursorUp.isUserInteractionEnabled = false
        cursorDown.isUserInteractionEnabled = false
        cursorLeft.isUserInteractionEnabled = false
        cursorRight.isUserInteractionEnabled = false
        cursorContainer.isUserInteractionEnabled = false
        buttonA.isUserInteractionEnabled = false
        buttonB.isUserInteractionEnabled = false
        buttonContainer.isUserInteractionEnabled = false
        ctrlStart.isUserInteractionEnabled = false
        ctrlSelect.isUserInteractionEnabled = false
        ctrlContainer.isUserInteractionEnabled = false
    }
    
    private func setupLabel(label: UILabel, text: String) {
        label.font = UIFont.systemFont(ofSize: 12.0)
        label.textColor = UIColor(red: 0.874509803921569,
                                  green: 0.443137254901961,
                                  blue: 0.149019607843137,
                                  alpha: 1.0)
        label.text = text
        label.isUserInteractionEnabled = false
    }
    
    override var frame: CGRect {
        didSet {
            var x = 0.0;
            var h = labelStart.intrinsicContentSize.height + 44;
            var y = frame.size.height - h;
            var w = frame.size.width;
            ctrlContainer.frame = CGRectMake(x, y, w, h);
            w = 88;
            h = 44;
            x = (ctrlContainer.frame.size.width - w * 2 - 16) / 2;
            y = ctrlContainer.frame.size.height - h;
            ctrlSelect.frame = CGRectMake(x, y, w, h);
            x += w + 16;
            ctrlStart.frame = CGRectMake(x, y, w, h);
            w = labelSelect.intrinsicContentSize.width;
            h = labelSelect.intrinsicContentSize.height;
            x = ctrlSelect.frame.origin.x + (88 - w) / 2;
            y = 0;
            labelSelect.frame = CGRectMake(x, y, w, h);
            w = labelStart.intrinsicContentSize.width;
            h = labelStart.intrinsicContentSize.height;
            x = ctrlStart.frame.origin.x + (88 - w) / 2;
            y = 0;
            labelStart.frame = CGRectMake(x, y, w, h);
            
            let cs = (frame.size.width - 8 * 3) / 2;
            w = cs;
            h = cs;
            x = 8;
            y = (frame.size.height - ctrlContainer.frame.size.height - h) / 2;
            cursorContainer.frame = CGRectMake(x, y, w, h);
            x += w + 8;
            buttonContainer.frame = CGRectMake(x, y, w, h);
            
            w = 48;
            h = 48;
            x = (cs - w) / 2;
            y = 8;
            cursorUp.frame = CGRectMake(x, y, w, h);
            y = cs - h - 8;
            cursorDown.frame = CGRectMake(x, y, w, h);
            y = x;
            x = 8;
            cursorLeft.frame = CGRectMake(x, y, w, h);
            x = (cs / 2 - w) / 2;
            buttonB.frame = CGRectMake(x, y, w, h);
            x = cs - w - 8;
            cursorRight.frame = CGRectMake(x, y, w, h);
            x = cs / 2 + (cs / 2 - w) / 2;
            buttonA.frame = CGRectMake(x, y, w, h);
            
            w = labelB.intrinsicContentSize.width;
            h = labelB.intrinsicContentSize.height;
            x = buttonB.frame.origin.x + (48 - w) / 2;
            y = buttonB.frame.origin.y - h - 8;
            labelB.frame = CGRectMake(x, y, w, h);
            
            w = labelA.intrinsicContentSize.width;
            h = labelA.intrinsicContentSize.height;
            x = buttonA.frame.origin.x + (48 - w) / 2;
            y = buttonA.frame.origin.y - h - 8;
            labelA.frame = CGRectMake(x, y, w, h);
        }
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        touches.forEach { touch in
            let pos = touch.location(in: self)
            if (CGRectContainsPoint(cursorContainer.frame, pos)) {
                self.cursorTouches.append(touch)
            } else if (CGRectContainsPoint(buttonContainer.frame, pos)) {
                self.buttonTouches.append(touch)
            } else if (CGRectContainsPoint(ctrlContainer.frame, pos)) {
                self.ctrlTouches.append(touch)
            }
        }
        hitTest()
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        hitTest()
    }
    
    private func remove(touches: Set<UITouch>) {
        touches.forEach { touch in
            cursorTouches.removeAll(where: {$0 == touch })
            buttonTouches.removeAll(where: {$0 == touch })
            ctrlTouches.removeAll(where: {$0 == touch })
        }
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        remove(touches: touches)
        hitTest()
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        remove(touches: touches)
        hitTest()
    }
    
    private func hitTest() {
        let up = joyPad.up
        let down = joyPad.down
        let left = joyPad.left
        let right = joyPad.right
        let a = joyPad.a
        let b = joyPad.b
        let start = joyPad.start
        let select = joyPad.select
        let previousCode = joyPad.code

        joyPad.up = false
        joyPad.down = false
        joyPad.left = false
        joyPad.right = false
        var cx = cursorContainer.frame.size.width / 2;
        var cy = cursorContainer.frame.size.height / 2;
        cursorTouches.forEach { touch in
            var ptr = touch.location(in: cursorContainer)
            if (abs(ptr.x - cx) >= 8 || abs(ptr.y - cy) >= 8) {
                var r = atan2f(Float(ptr.x - cx), Float(ptr.y - cy)) + Float.pi / 2;
                while (Float.pi * 2 <= r) {
                    r -= Float.pi * 2;
                }
                while (r < 0) {
                    r += Float.pi * 2;
                }
                joyPad.down = 0.5235987755983 <= r && r <= 2.6179938779915;
                joyPad.right = 2.2689280275926 <= r && r <= 4.1887902047864;
                joyPad.up = 3.6651914291881 <= r && r <= 5.7595865315813;
                joyPad.left = r <= 1.0471975511966 || 5.235987755983 <= r;
            }
        }

        joyPad.a = false
        joyPad.b = false
        let bE = buttonB.frame.origin.x + buttonB.frame.size.width + 4;
        let aS = buttonA.frame.origin.x - 4;
        buttonTouches.forEach { touch in
            let x = touch.location(in: buttonContainer).x
            if (x < bE) {
                joyPad.b = true;
            } else if (aS < x) {
                joyPad.a = true;
            } else {
                joyPad.b = true;
                joyPad.a = true;
            }
        }

        joyPad.select = false
        joyPad.start = false
        ctrlTouches.forEach { touch in
            let pos = touch.location(in: ctrlContainer)
            if (CGRectContainsPoint(ctrlStart.frame, pos)) {
                joyPad.start = true;
            } else if (CGRectContainsPoint(ctrlSelect.frame, pos)) {
                joyPad.select = true;
            }
        }

        if (joyPad.code == previousCode) {
            return
        }

        if (up != joyPad.up) {
            if (joyPad.up) {
                cursorUp.image = UIImage(named: "pad_up_on")
            } else {
                cursorUp.image = UIImage(named: "pad_up_off")
            }
        }

        if (down != joyPad.down) {
            if (joyPad.down) {
                cursorDown.image = UIImage(named: "pad_down_on")
            } else {
                cursorDown.image = UIImage(named: "pad_down_off")
            }
        }

        if (left != joyPad.left) {
            if (joyPad.left) {
                cursorLeft.image = UIImage(named: "pad_left_on")
            } else {
                cursorLeft.image = UIImage(named: "pad_left_off")
            }
        }

        if (right != joyPad.right) {
            if (joyPad.right) {
                cursorRight.image = UIImage(named: "pad_right_on")
            } else {
                cursorRight.image = UIImage(named: "pad_right_off")
            }
        }

        if (a != joyPad.a) {
            if (joyPad.a) {
                buttonA.image = UIImage(named: "pad_btn_on")
            } else {
                buttonA.image = UIImage(named: "pad_btn_off")
            }
        }

        if (b != joyPad.b) {
            if (joyPad.b) {
                buttonB.image = UIImage(named: "pad_btn_on")
            } else {
                buttonB.image = UIImage(named: "pad_btn_off")
            }
        }

        if (start != joyPad.start) {
            if (joyPad.start) {
                ctrlStart.image = UIImage(named: "pad_ctrl_on")
            } else {
                ctrlStart.image = UIImage(named: "pad_ctrl_off")
            }
        }

        if (select != joyPad.select) {
            if (joyPad.select) {
                ctrlSelect.image = UIImage(named: "pad_ctrl_on")
            } else {
                ctrlSelect.image = UIImage(named: "pad_ctrl_off")
            }
        }
    }
}
