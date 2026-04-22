// vim: set ts=2 sw=2 ai et:
// SPDX-License-Identifier: MIT
// Copyright (C) 2026 Madeleine Choi

import Foundation

final class DiscoveryDelegate: NSObject, NetServiceBrowserDelegate, NetServiceDelegate {

  private let all: UnsafeMutablePointer<all_services_t>
  private var services: [NetService] = []

  init(all: UnsafeMutablePointer<all_services_t>) {
    self.all = all
  }

  func netServiceBrowser(
    _ browser: NetServiceBrowser,
    didFind service: NetService,
    moreComing: Bool
  ) {

    service.delegate = self
    services.append(service)
    service.resolve(withTimeout: 5.0)

    if !moreComing {
      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        CFRunLoopStop(CFRunLoopGetCurrent())
      }
    }
  }

  func netServiceDidResolveAddress(_ sender: NetService) {

    let host = sender.hostName
    let port = sender.port

    guard let txt = sender.txtRecordData() else { return }

    let dict = NetService.dictionary(fromTXTRecord: txt)

    func string(_ key: String) -> String? {
      guard let data = dict[key] else { return nil }
      return String(data: data, encoding: .utf8)
    }

    let idStr = string("id")
    let nameStr = string("md")
    let sfString = string("sf")

    let status: accessory_status_t =
      sfString
      .flatMap { UInt32($0) }
      .map { accessory_status_t($0) }
      ?? ACCESSORY_STATUS_UNPAIRED

    let current = all.pointee

    if current.count == current.capacity {

      let newCapacity = current.capacity * 2

      let newRaw = realloc(
        current.items,
        newCapacity * MemoryLayout<UnsafeMutablePointer<accessory_t>?>.size
      )

      guard let newRaw else { return }

      let typed = newRaw.assumingMemoryBound(
        to: UnsafeMutablePointer<accessory_t>?.self
      )

      all.pointee.items = typed
      all.pointee.capacity = newCapacity
    }

    let acc = UnsafeMutablePointer<accessory_t>.allocate(capacity: 1)
    acc.initialize(
      to: accessory_t(
        id: strdup(idStr ?? ""),
        name: strdup(nameStr ?? ""),
        address: strdup(host ?? ""),
        status: status,
        port: UInt16(port),
      ))

    all.pointee.items[Int(all.pointee.count)] = UnsafeMutablePointer(mutating: acc)
    all.pointee.count += 1
  }
}

@_cdecl("all_services_discover")
public func all_services_discover() -> UnsafeMutablePointer<all_services_t>? {

  let all = UnsafeMutablePointer<all_services_t>.allocate(capacity: 1)

  all.initialize(
    to: all_services_t(
      items: nil,
      count: 0,
      capacity: 8
    ))

  all.pointee.items = UnsafeMutablePointer<UnsafeMutablePointer<accessory_t>?>.allocate(capacity: 8)

  let delegate = DiscoveryDelegate(all: all)

  let browser = NetServiceBrowser()
  browser.delegate = delegate
  browser.searchForServices(ofType: "_hap._tcp.", inDomain: "")

  CFRunLoopRun()

  browser.stop()

  return all
}
