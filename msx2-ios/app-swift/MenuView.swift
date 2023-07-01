//
//  MenuView.swift
//  app-swift
//
//  Created by Yoji Suzuki on 2023/07/01.
//

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
