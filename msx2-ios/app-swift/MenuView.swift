/**
 * micro MSX2+ - Example for iOS Swift
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Yoji Suzuki.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
import UIKit

protocol MenuViewDelegate : NSObject {
    func menuViewDidPushResetButton()
}

class MenuView: UIView {
    weak var delegate: MenuViewDelegate? = nil
    private let label = UILabel()
    private let resetButton = UIButton(type: .roundedRect)
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setup()
    }

    override init(frame: CGRect) {
        super.init(frame: frame)
        setup()
    }

    private func setup() {
        label.font = UIFont.boldSystemFont(ofSize: 16.0)
        label.textColor = .white
        label.textAlignment = .left
        label.text = "micro MSX2+ - Example (Swift)"
        resetButton.setTitle("Reset", for: .normal)
        resetButton.setTitleColor(.white, for: .normal)
        resetButton.addAction(.init { _ in self.delegate?.menuViewDidPushResetButton() }, for: .touchUpInside)
        addSubview(label)
        addSubview(resetButton)
    }

    override var frame: CGRect {
        didSet {
            var w = resetButton.intrinsicContentSize.width
            var h = resetButton.intrinsicContentSize.height
            var x = frame.size.width - w - 16
            var y = (frame.size.height - h) / 2
            resetButton.frame = CGRectMake(x, y, w, h)
            w = x - 8
            x = 8
            h = label.intrinsicContentSize.height
            y = (frame.size.height - h) / 2
            label.frame = CGRectMake(x, y, w, h)
        }
    }
}
