Feature: Import iSearch Profiles
  Background:
    Given I am logged in as a user with the "administrator" role

    @private_files @javascript @api @panopoly_magic @drushTest
      Scenario: Import iSearch Profiles
      Given I am at "/admin/content/isearch/configure"
      When I click on the property "data-reactid" from the element "label" with value ".1.1"
      And I press the "Browse" button
      And I click on the property "dept_nid" from the element "li" with value "1351"
      And I click on the property "data-reactid" from the element "label" with value ".0.2.0.2.0.1"
      And I press the "Submit" button
      And I press the "Save configuration" button
      And I click "Import iSearch Profiles"
      And I fill in "edit-isearch-import-limit-value" with "50"
      And I press the "Begin import" button
      And I wait for 20 seconds
      Then I should see "Processed"

  @private_files @javascript @api @panopoly_magic @drushTest
  Scenario: Add directory panel with imported profiles
    Given I am at "/node/add"
    And I click "Content Page"
    And I fill in "Title" with "Test page for directory panel"
    When I type "testing in body" in the "edit-body-und-0-value" WYSIWYG editor
    And I press the "Publish" button
    And I click "Change layout"
    And I click "Boxton"
    And I press the "Save as Content Page default" button
    And I click "Customize this page"
    And I click "Add new pane"
    And I click "Add ASU Directory Panel"
    And I press the "Browse" button
    And I click on the property "dept_nid" from the element "li" with value "1351"
    And I click on the text " Include sub-departments?" in the "label" tag
    And I press the "Submit" button
    And I click "Advanced"
    And I click on the property "for" from the element "label" with value "edit-field-asu-directory-items-und-0-horizontal-tabs-advanced-always-display-view"
    And I press the "Add" button
    And I press the "Save as custom" button
    And I click on the text "View" in the "a" tag
    And I click on the text "ALL" in the "li" tag
    Then I should not see "No results found."