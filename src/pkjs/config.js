module.exports = [
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Lazy Watch Colors" },
      {
        "type": "color",
        "messageKey": "backgroundColor",
        "label": "Background",
        "defaultValue": "000000",
        "sunlight": true,
        "layout": "COLOR"
      },
      {
        "type": "color",
        "messageKey": "textColor",
        "label": "Text",
        "defaultValue": "FFFFFF",
        "sunlight": true,
        "layout": "COLOR"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Layout" },
      {
        "type": "toggle",
        "messageKey": "blockAlign",
        "label": "Align by longest word",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Readability" },
      {
        "type": "select",
        "messageKey": "wordStyle",
        "label": "\"vor\" / \"nach\" / \"Uhr\" style",
        "defaultValue": "bold",
        "options": [
          { "label": "Bold", "value": "bold" },
          { "label": "Normal", "value": "normal" },
          { "label": "Italic", "value": "italic" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Night Mode" },
      {
        "type": "toggle",
        "messageKey": "nightModeEnabled",
        "label": "Enable night mode",
        "defaultValue": false
      },
      {
        "type": "input",
        "messageKey": "nightModeStart",
        "label": "Starts at",
        "defaultValue": "22:00",
        "attributes": { "type": "time" }
      },
      {
        "type": "input",
        "messageKey": "nightModeEnd",
        "label": "Ends at",
        "defaultValue": "06:00",
        "attributes": { "type": "time" }
      },
      {
        "type": "color",
        "messageKey": "nightBackgroundColor",
        "label": "Night background",
        "defaultValue": "000000",
        "sunlight": true,
        "layout": "COLOR"
      },
      {
        "type": "color",
        "messageKey": "nightTextColor",
        "label": "Night text",
        "defaultValue": "550000",
        "sunlight": true,
        "layout": "COLOR"
      }
    ]
  },
  { "type": "submit", "defaultValue": "Save" }
];
