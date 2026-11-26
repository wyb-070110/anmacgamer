// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The menu bar item and notices on macOS. showIcon puts the app's icon in the menu bar (NSStatusItem); a click
// opens its menu, or calls activated when there is no menu. show() posts a notification through the notification
// center; that needs the app bundle (outside one it reports false) and the user's permission, asked once.
#include "gs/platform/TrayNotice.h"

#import <AppKit/AppKit.h>
#import <UserNotifications/UserNotifications.h>

#include <vector>

namespace gs::platform {

struct TrayNotice::Impl {
    std::function<void()> clicked;
    std::function<void()> activated;
    std::vector<TrayMenuItem> items;
    NSStatusItem* item = nil;
    id target = nil;
};

const std::function<void()>* trayActivated(void* owner)
{
    return owner != nullptr ? &static_cast<TrayNotice*>(owner)->m_impl->activated : nullptr;
}

void trayChose(void* owner, std::size_t index)
{
    if (owner == nullptr) {
        return;
    }
    const std::vector<TrayMenuItem>& items = static_cast<TrayNotice*>(owner)->m_impl->items;
    if (index < items.size() && items[index].chosen) {
        items[index].chosen();
    }
}

}

@interface GSTrayTarget : NSObject
@property(nonatomic, assign) void* owner;
@end

@implementation GSTrayTarget
- (void)clicked:(id)sender
{
    if (const std::function<void()>* activated = gs::platform::trayActivated(self.owner); activated && *activated) {
        (*activated)();
    }
}
- (void)chose:(NSMenuItem*)sender
{
    gs::platform::trayChose(self.owner, static_cast<std::size_t>(sender.tag));
}
@end

namespace gs::platform {

namespace {

NSString* nsText(const std::wstring& text)
{
    NSString* made = [[NSString alloc] initWithBytes:text.data()
                                    length:text.size() * sizeof(wchar_t)
                                  encoding:NSUTF32LittleEndianStringEncoding];
    return made != nil ? made : @"";
}

bool inBundle()
{
    return [[NSBundle mainBundle] bundleIdentifier] != nil;
}

}

TrayNotice::TrayNotice(std::function<void()> clicked) : m_impl(std::make_unique<Impl>())
{
    m_impl->clicked = std::move(clicked);
    GSTrayTarget* target = [[GSTrayTarget alloc] init];
    target.owner = this;
    m_impl->target = target;
}

TrayNotice::~TrayNotice()
{
    if (m_impl->item != nil) {
        [[NSStatusBar systemStatusBar] removeStatusItem:m_impl->item];
    }
}

bool TrayNotice::show(const std::wstring& title, const std::wstring& text)
{
    if (!inBundle()) {
        return false;
    }
    @autoreleasepool {
        UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
        UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
        content.title = nsText(title);
        content.body = nsText(text);
        UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:[[NSUUID UUID] UUIDString]
                                                                              content:content
                                                                              trigger:nil];
        [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert | UNAuthorizationOptionSound
                              completionHandler:^(BOOL granted, NSError*) {
                                  if (granted) {
                                      [center addNotificationRequest:request withCompletionHandler:nil];
                                  }
                              }];
        return true;
    }
}

bool TrayNotice::showIcon()
{
    @autoreleasepool {
        if (m_impl->item == nil) {
            m_impl->item = [[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength];
            NSImage* icon = [[NSApp applicationIconImage] copy];
            icon.size = NSMakeSize(18, 18);
            m_impl->item.button.image = icon;
            m_impl->item.button.target = m_impl->target;
            m_impl->item.button.action = @selector(clicked:);
            setMenu(m_impl->items);
        }
        return m_impl->item != nil;
    }
}

void TrayNotice::setActivated(std::function<void()> activated)
{
    m_impl->activated = std::move(activated);
}

void TrayNotice::setMenu(std::vector<TrayMenuItem> items)
{
    m_impl->items = std::move(items);
    if (m_impl->item == nil) {
        return;
    }
    @autoreleasepool {
        if (m_impl->items.empty()) {
            m_impl->item.menu = nil;
            return;
        }
        NSMenu* menu = [[NSMenu alloc] init];
        menu.autoenablesItems = NO;
        for (std::size_t index = 0; index < m_impl->items.size(); ++index) {
            const TrayMenuItem& entry = m_impl->items[index];
            if (entry.separator) {
                [menu addItem:[NSMenuItem separatorItem]];
                continue;
            }
            NSMenuItem* made = [[NSMenuItem alloc] initWithTitle:nsText(entry.text)
                                                          action:@selector(chose:)
                                                   keyEquivalent:@""];
            made.target = m_impl->target;
            made.tag = static_cast<NSInteger>(index);
            made.enabled = entry.enabled;
            made.state = entry.checked ? NSControlStateValueOn : NSControlStateValueOff;
            [menu addItem:made];
        }
        m_impl->item.menu = menu;
    }
}

}
