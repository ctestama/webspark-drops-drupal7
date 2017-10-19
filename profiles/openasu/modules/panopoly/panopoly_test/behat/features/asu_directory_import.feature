Feature: Import iSearch Profiles
  Background:
    Given I am logged in as a user with the "administrator" role

    @private_files @javascript @api @panopoly_magic @drushTest
      Scenario: Import iSearch Profiles
      Given I am at "/admin/content/isearch/configure"
      When I check the box "edit-isearch-display-affiliations"
      And I press the "Browse" button
      And I click on the department "1351"
      And I click on the box text ".0.2.0.2.0.1"
      And I press the "Submit" button
      And I press the "Save configuration" button
      And I click "Import iSearch Profiles"
      And I fill in "edit-isearch-import-limit-value" with "50"
      And I press the "Begin import" button
      And I wait for 30 seconds
      Then I should see "The configuration options have been saved."