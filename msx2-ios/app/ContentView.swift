//
//  ContentView.swift
//  app
//
//  Created by Yoji Suzuki on 2023/06/27.
//

import SwiftUI
import MSX2

struct ContentView: View {
    var body: some View {
        VStack {
            Image(systemName: "globe")
                .imageScale(.large)
                .foregroundColor(.accentColor)
            Text("Hello, world!")
        }
        .padding()
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
