// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.piet;

import static com.google.android.libraries.feed.common.testing.RunnableSubject.assertThatRunnable;
import static com.google.common.truth.Truth.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import com.google.android.libraries.feed.api.host.config.DebugBehavior;
import com.google.android.libraries.feed.common.functional.Suppliers;
import com.google.android.libraries.feed.common.time.testing.FakeClock;
import com.google.android.libraries.feed.piet.PietStylesHelper.PietStylesHelperFactory;
import com.google.android.libraries.feed.piet.TemplateBinder.TemplateAdapterModel;
import com.google.android.libraries.feed.piet.TemplateBinder.TemplateKey;
import com.google.android.libraries.feed.piet.host.ActionHandler;
import com.google.android.libraries.feed.piet.host.AssetProvider;
import com.google.search.now.ui.piet.BindingRefsProto.ParameterizedTextBindingRef;
import com.google.search.now.ui.piet.ElementsProto.BindingContext;
import com.google.search.now.ui.piet.ElementsProto.BindingValue;
import com.google.search.now.ui.piet.ElementsProto.Content;
import com.google.search.now.ui.piet.ElementsProto.Element;
import com.google.search.now.ui.piet.ElementsProto.ElementList;
import com.google.search.now.ui.piet.ElementsProto.GridRow;
import com.google.search.now.ui.piet.ElementsProto.TextElement;
import com.google.search.now.ui.piet.GradientsProto.Fill;
import com.google.search.now.ui.piet.PietProto.Frame;
import com.google.search.now.ui.piet.PietProto.PietSharedState;
import com.google.search.now.ui.piet.PietProto.Stylesheet;
import com.google.search.now.ui.piet.PietProto.Stylesheets;
import com.google.search.now.ui.piet.PietProto.Template;
import com.google.search.now.ui.piet.StylesProto.Style;
import com.google.search.now.ui.piet.StylesProto.StyleIdsStack;
import com.google.search.now.ui.piet.TextProto.ParameterizedText;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;

/** Tests for TemplateBinder methods. */
@RunWith(RobolectricTestRunner.class)
public class TemplateBinderTest {
  private static final int FRAME_COLOR = 12345;
  private static final String FRAME_STYLESHEET_ID = "coolcat";
  private static final String TEXT_STYLE_ID = "catalog";
  private static final String TEMPLATE_ID = "duplicat";
  private static final String OTHER_TEMPLATE_ID = "alleycat";
  private static final String TEXT_BINDING_ID = "ofmiceandmen";
  private static final String TEXT_CONTENTS = "afewmilessouth";

  private static final int FRAME_WIDTH_PX = 321;

  private static final Template MODEL_TEMPLATE =
      Template.newBuilder()
          .setTemplateId(TEMPLATE_ID)
          .setElement(
              Element.newBuilder()
                  .setStyleReferences(StyleIdsStack.newBuilder().addStyleIds(TEXT_STYLE_ID))
                  .setTextElement(
                      TextElement.newBuilder()
                          .setParameterizedTextBinding(
                              ParameterizedTextBindingRef.newBuilder()
                                  .setBindingId(TEXT_BINDING_ID))))
          .build();
  private static final Template OTHER_TEMPLATE =
      Template.newBuilder()
          .setTemplateId(OTHER_TEMPLATE_ID)
          .setElement(Element.newBuilder().setElementList(ElementList.getDefaultInstance()))
          .build();

  private static final Frame DEFAULT_FRAME =
      Frame.newBuilder()
          .setStylesheets(
              Stylesheets.newBuilder()
                  .addStylesheets(
                      Stylesheet.newBuilder()
                          .setStylesheetId(FRAME_STYLESHEET_ID)
                          .addStyles(
                              Style.newBuilder().setStyleId(TEXT_STYLE_ID).setColor(FRAME_COLOR))))
          .addTemplates(MODEL_TEMPLATE)
          .addTemplates(OTHER_TEMPLATE)
          .build();

  private static final BindingContext MODEL_BINDING_CONTEXT =
      BindingContext.newBuilder()
          .addBindingValues(
              BindingValue.newBuilder()
                  .setBindingId(TEXT_BINDING_ID)
                  .setParameterizedText(ParameterizedText.newBuilder().setText(TEXT_CONTENTS)))
          .build();

  private static final Element DEFAULT_TEMPLATE_ELEMENT =
      Element.newBuilder().setElementList(ElementList.getDefaultInstance()).build();

  @Mock private ElementListAdapter elementListAdapter;
  @Mock private ElementAdapter<? extends View, ?> templateAdapter;
  @Mock private FrameContext frameContext;
  @Mock private ElementAdapterFactory adapterFactory;
  @Mock private ActionHandler actionHandler;

  private KeyedRecyclerPool<ElementAdapter<? extends View, ?>> templateRecyclerPool;

  private TemplateBinder templateBinder;

  @Before
  public void setUp() throws Exception {
    initMocks(this);

    templateRecyclerPool = new KeyedRecyclerPool<>(100, 100);
    templateBinder = new TemplateBinder(templateRecyclerPool, adapterFactory);
  }

  @Test
  public void testCreateTemplateAdapter() {
    // Set up data for test: template, shared states, binding context, template adapter model
    String templateId = "papa";
    Template template =
        Template.newBuilder()
            .setTemplateId(templateId)
            .setElement(DEFAULT_TEMPLATE_ELEMENT)
            .build();
    when(frameContext.getTemplate(templateId)).thenReturn(template);
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template).build();
    List<PietSharedState> sharedStates = Collections.singletonList(sharedState);
    when(frameContext.getPietSharedStates()).thenReturn(sharedStates);
    BindingContext bindingContext =
        BindingContext.newBuilder()
            .addBindingValues(BindingValue.newBuilder().setBindingId("potato"))
            .build();
    TemplateAdapterModel model = new TemplateAdapterModel(template, bindingContext);

    // Set frameContext to return a new frameContext when a template is bound
    FrameContext templateContext = mock(FrameContext.class);
    when(frameContext.createTemplateContext(template, bindingContext)).thenReturn(templateContext);
    doReturn(elementListAdapter)
        .when(adapterFactory)
        .createAdapterForElement(DEFAULT_TEMPLATE_ELEMENT, templateContext);

    ElementAdapter<? extends View, ?> adapter =
        templateBinder.createTemplateAdapter(model, frameContext);

    assertThat(adapter).isSameInstanceAs(elementListAdapter);
    verify(elementListAdapter).setKey(new TemplateKey(template, sharedStates, new ArrayList<>()));
  }

  @Test
  public void testCreateTemplateAdapter_recycled() {
    Template template =
        Template.newBuilder()
            .setTemplateId("template")
            .setElement(DEFAULT_TEMPLATE_ELEMENT)
            .build();
    List<PietSharedState> sharedStates = Collections.emptyList();
    TemplateKey templateKey = new TemplateKey(template, sharedStates, new ArrayList<>());
    when(templateAdapter.getKey()).thenReturn(templateKey);
    when(frameContext.getPietSharedStates()).thenReturn(sharedStates);

    // Release adapter to populate the recycler pool
    templateRecyclerPool.put(templateKey, templateAdapter);

    // Get a new adapter from the pool.
    TemplateAdapterModel model =
        new TemplateAdapterModel(template, BindingContext.getDefaultInstance());
    ElementAdapter<? extends View, ?> adapter =
        templateBinder.createTemplateAdapter(model, frameContext);

    assertThat(adapter).isSameInstanceAs(templateAdapter);
    // We don't need to re-create the adapter; it has already been created.
    verify(templateAdapter, never()).createAdapter(any(Element.class), any());
    verify(templateAdapter, never()).createAdapter(any(), any(), any());
  }

  /**
   * Set up a "real" environment, and ensure that styles are set from the template, not from the
   * frame.
   */
  @Test
  public void testCreateTemplateAdapter_checkStyleSources() {
    // Set up 3 styles: one on the template, one on the frame, and one on the shared state.
    String templateStyleId = "templateStyle";
    String frameStyleId = "frameStyle";
    String globalStyleId = "globalStyle";

    int templateColor = Color.GREEN;
    Style templateStyle =
        Style.newBuilder()
            .setStyleId(templateStyleId)
            .setBackground(Fill.newBuilder().setColor(templateColor))
            .build();
    int frameColor = Color.RED;
    Style frameStyle =
        Style.newBuilder()
            .setStyleId(frameStyleId)
            .setBackground(Fill.newBuilder().setColor(frameColor))
            .build();
    int globalColor = Color.BLUE;
    Style globalStyle =
        Style.newBuilder()
            .setStyleId(globalStyleId)
            .setBackground(Fill.newBuilder().setColor(globalColor))
            .build();

    // Style on the frame with the same ID as the template's style.
    int frameTemplateColor = Color.MAGENTA;
    Style frameTemplateStyle =
        Style.newBuilder()
            .setStyleId(templateStyleId)
            .setBackground(Fill.newBuilder().setColor(frameTemplateColor))
            .build();

    // Template: A list of 3 elements with template style, frame style, and global style,
    // respectively.
    Template template =
        Template.newBuilder()
            .setStylesheets(
                Stylesheets.newBuilder()
                    .addStylesheets(Stylesheet.newBuilder().addStyles(templateStyle)))
            .setElement(
                Element.newBuilder()
                    .setElementList(
                        ElementList.newBuilder()
                            .addContents(
                                Content.newBuilder()
                                    .setElement(
                                        Element.newBuilder()
                                            .setStyleReferences(
                                                StyleIdsStack.newBuilder()
                                                    .addStyleIds(templateStyleId))
                                            .setElementList(ElementList.getDefaultInstance())))
                            .addContents(
                                Content.newBuilder()
                                    .setElement(
                                        Element.newBuilder()
                                            .setStyleReferences(
                                                StyleIdsStack.newBuilder()
                                                    .addStyleIds(frameStyleId))
                                            .setElementList(ElementList.getDefaultInstance())))
                            .addContents(
                                Content.newBuilder()
                                    .setElement(
                                        Element.newBuilder()
                                            .setStyleReferences(
                                                StyleIdsStack.newBuilder()
                                                    .addStyleIds(globalStyleId))
                                            .setElementList(ElementList.getDefaultInstance())))))
            .build();

    PietSharedState pietSharedState =
        PietSharedState.newBuilder()
            .addTemplates(template)
            .addStylesheets(Stylesheet.newBuilder().addStyles(globalStyle))
            .build();

    // Frame defines style IDs that are also defined in the template
    Frame frame =
        Frame.newBuilder()
            .setStylesheets(
                Stylesheets.newBuilder()
                    .addStylesheets(
                        Stylesheet.newBuilder()
                            .addStyles(frameStyle)
                            .addStyles(frameTemplateStyle)))
            .build();

    // Set up a "real" frameContext, adapterParameters, factory
    Context context = Robolectric.buildActivity(Activity.class).get();
    List<PietSharedState> pietSharedStates = Collections.singletonList(pietSharedState);
    HostProviders mockHostProviders = mock(HostProviders.class);
    AssetProvider mockAssetProvider = mock(AssetProvider.class);
    when(mockHostProviders.getAssetProvider()).thenReturn(mockAssetProvider);
    MediaQueryHelper mediaQueryHelper =
        new MediaQueryHelper(FRAME_WIDTH_PX, mockAssetProvider, context);
    PietStylesHelper pietStylesHelper =
        new PietStylesHelperFactory().get(pietSharedStates, mediaQueryHelper);
    frameContext =
        FrameContext.createFrameContext(
            frame,
            pietSharedStates,
            pietStylesHelper,
            DebugBehavior.VERBOSE,
            new DebugLogger(),
            actionHandler,
            mockHostProviders,
            new FrameLayout(context));
    AdapterParameters adapterParameters =
        new AdapterParameters(
            context, Suppliers.of(null), mockHostProviders, new FakeClock(), false, false);
    TemplateBinder templateBinder = adapterParameters.templateBinder;

    // Create and bind adapter
    TemplateAdapterModel templateModel =
        new TemplateAdapterModel(template, BindingContext.getDefaultInstance());
    ElementAdapter<? extends View, ?> adapter =
        templateBinder.createTemplateAdapter(templateModel, frameContext);
    templateBinder.bindTemplateAdapter(adapter, templateModel, frameContext);

    // Check views to ensure that template styles get set, but not frame or global styles.
    LinearLayout templateList = (LinearLayout) adapter.getView();
    assertThat(templateList.getChildCount()).isEqualTo(3);

    // Template style element gets background from template
    assertThat(((ColorDrawable) templateList.getChildAt(0).getBackground()).getColor())
        .isEqualTo(templateColor);

    // Frame style element gets no background - frame styles are not in scope for template.
    assertThat(templateList.getChildAt(1).getBackground()).isNull();

    // Global style element gets no background - global styles are not in scope for template.
    assertThat(templateList.getChildAt(2).getBackground()).isNull();
  }

  @Test
  public void testBindTemplateAdapter_success() {
    // Set up data for test: template, shared states, binding context, template adapter model
    String templateId = "papa";
    Template template =
        Template.newBuilder()
            .setTemplateId(templateId)
            .setElement(Element.newBuilder().setElementList(ElementList.getDefaultInstance()))
            .build();
    when(frameContext.getTemplate(templateId)).thenReturn(template);
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template).build();
    List<PietSharedState> sharedStates = Collections.singletonList(sharedState);
    when(frameContext.getPietSharedStates()).thenReturn(sharedStates);
    BindingContext bindingContext =
        BindingContext.newBuilder()
            .addBindingValues(BindingValue.newBuilder().setBindingId("potato"))
            .build();
    TemplateAdapterModel model = new TemplateAdapterModel(template, bindingContext);

    // Set frameContext to return a new frameContext when a template is bound
    FrameContext templateContext = mock(FrameContext.class);
    when(frameContext.createTemplateContext(template, bindingContext)).thenReturn(templateContext);
    doReturn(elementListAdapter)
        .when(adapterFactory)
        .createAdapterForElement(DEFAULT_TEMPLATE_ELEMENT, templateContext);

    // Create adapter and ensure template key is set.
    ElementAdapter<? extends View, ?> adapter =
        templateBinder.createTemplateAdapter(model, frameContext);
    ArgumentCaptor<RecyclerKey> keyArgumentCaptor = ArgumentCaptor.forClass(RecyclerKey.class);
    verify(elementListAdapter).setKey(keyArgumentCaptor.capture());
    when(elementListAdapter.getKey()).thenReturn(keyArgumentCaptor.getValue());

    templateBinder.bindTemplateAdapter(adapter, model, frameContext);

    // Assert that adapter is bound with the template frameContext
    verify(elementListAdapter).bindModel(model.getTemplate().getElement(), templateContext);
  }

  @Test
  public void testBindTemplateAdapter_nullKey() {
    TemplateAdapterModel templateAdapterModel =
        new TemplateAdapterModel(Template.getDefaultInstance(), null);
    when(elementListAdapter.getKey()).thenReturn(null);

    assertThatRunnable(
            () ->
                templateBinder.bindTemplateAdapter(
                    elementListAdapter, templateAdapterModel, frameContext))
        .throwsAnExceptionOfType(NullPointerException.class)
        .that()
        .hasMessageThat()
        .contains("Adapter key was null");
  }

  @Test
  public void testBindTemplateAdapter_notATemplateAdapter() {
    TemplateAdapterModel templateAdapterModel =
        new TemplateAdapterModel(Template.getDefaultInstance(), null);
    when(elementListAdapter.getKey()).thenReturn(ElementListAdapter.KeySupplier.SINGLETON_KEY);

    assertThatRunnable(
            () ->
                templateBinder.bindTemplateAdapter(
                    elementListAdapter, templateAdapterModel, frameContext))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("bindTemplateAdapter only applicable for template adapters");
  }

  @Test
  public void testBindTemplateAdapter_templateMismatch() {
    // Set up data for test: two templates, shared states, binding context, template adapter model
    String templateId1 = "papa";
    Template template1 =
        Template.newBuilder()
            .setTemplateId(templateId1)
            .setElement(DEFAULT_TEMPLATE_ELEMENT)
            .build();
    String templateId2 = "mama";
    Template template2 =
        Template.newBuilder()
            .setTemplateId(templateId2)
            .setElement(Element.newBuilder().setGridRow(GridRow.getDefaultInstance()))
            .build();
    when(frameContext.getTemplate(templateId1)).thenReturn(template1);
    when(frameContext.getTemplate(templateId2)).thenReturn(template2);
    PietSharedState sharedState =
        PietSharedState.newBuilder().addTemplates(template1).addTemplates(template2).build();
    List<PietSharedState> sharedStates = Collections.singletonList(sharedState);
    when(frameContext.getPietSharedStates()).thenReturn(sharedStates);
    BindingContext bindingContext =
        BindingContext.newBuilder()
            .addBindingValues(BindingValue.newBuilder().setBindingId("potato"))
            .build();
    TemplateAdapterModel model1 = new TemplateAdapterModel(template1, bindingContext);
    TemplateAdapterModel model2 = new TemplateAdapterModel(template2, bindingContext);
    assertThat(new TemplateKey(template1, sharedStates, new ArrayList<>()))
        .isNotEqualTo(new TemplateKey(template2, sharedStates, new ArrayList<>()));

    // Set frameContext to return a new frameContext when a template is bound
    FrameContext templateContext = mock(FrameContext.class);
    when(frameContext.createTemplateContext(template1, bindingContext)).thenReturn(templateContext);
    when(frameContext.createTemplateContext(template2, bindingContext)).thenReturn(templateContext);
    doReturn(elementListAdapter)
        .when(adapterFactory)
        .createAdapterForElement(DEFAULT_TEMPLATE_ELEMENT, templateContext);

    // Create adapter with first template and ensure template key is set.
    ElementAdapter<? extends View, ?> adapter =
        templateBinder.createTemplateAdapter(model1, frameContext);
    ArgumentCaptor<RecyclerKey> keyArgumentCaptor = ArgumentCaptor.forClass(RecyclerKey.class);
    verify(elementListAdapter).setKey(keyArgumentCaptor.capture());
    when(elementListAdapter.getKey()).thenReturn(keyArgumentCaptor.getValue());

    // Try to bind with a different template and fail.
    assertThatRunnable(() -> templateBinder.bindTemplateAdapter(adapter, model2, frameContext))
        .throwsAnExceptionOfType(IllegalStateException.class)
        .that()
        .hasMessageThat()
        .contains("Template keys did not match");
  }

  @Test
  public void testCreateAndBindTemplateAdapter_newAdapter() {
    // Set up data for test: template, shared states, binding context, template adapter model
    String templateId = "papa";
    Template template =
        Template.newBuilder()
            .setTemplateId(templateId)
            .setElement(DEFAULT_TEMPLATE_ELEMENT)
            .build();
    when(frameContext.getTemplate(templateId)).thenReturn(template);
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template).build();
    List<PietSharedState> sharedStates = Collections.singletonList(sharedState);
    when(frameContext.getPietSharedStates()).thenReturn(sharedStates);
    BindingContext bindingContext =
        BindingContext.newBuilder()
            .addBindingValues(BindingValue.newBuilder().setBindingId("potato"))
            .build();
    TemplateAdapterModel model = new TemplateAdapterModel(template, bindingContext);

    // Set frameContext to return a new frameContext when a template is bound
    FrameContext templateContext = mock(FrameContext.class);
    when(frameContext.createTemplateContext(template, bindingContext)).thenReturn(templateContext);
    doReturn(elementListAdapter)
        .when(adapterFactory)
        .createAdapterForElement(DEFAULT_TEMPLATE_ELEMENT, templateContext);

    // Create adapter and ensure template key is set.
    ElementAdapter<? extends View, ?> adapter =
        templateBinder.createAndBindTemplateAdapter(model, frameContext);

    assertThat(adapter).isSameInstanceAs(elementListAdapter);
    verify(elementListAdapter).setKey(new TemplateKey(template, sharedStates, new ArrayList<>()));

    // Assert that adapter is bound with the template frameContext
    verify(elementListAdapter).bindModel(model.getTemplate().getElement(), templateContext);
  }

  @Test
  public void testCreateAndBindTemplateAdapter_recycled() {
    // Set up data for test: template, shared states, binding context, template adapter model
    String templateId = "papa";
    Template template =
        Template.newBuilder()
            .setTemplateId(templateId)
            .setElement(DEFAULT_TEMPLATE_ELEMENT)
            .build();
    when(frameContext.getTemplate(templateId)).thenReturn(template);
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template).build();
    List<PietSharedState> sharedStates = Collections.singletonList(sharedState);
    when(frameContext.getPietSharedStates()).thenReturn(sharedStates);
    BindingContext bindingContext =
        BindingContext.newBuilder()
            .addBindingValues(BindingValue.newBuilder().setBindingId("potato"))
            .build();
    TemplateAdapterModel model = new TemplateAdapterModel(template, bindingContext);

    // Set frameContext to return a new frameContext when a template is bound
    FrameContext templateContext = mock(FrameContext.class);
    when(frameContext.createTemplateContext(template, bindingContext)).thenReturn(templateContext);
    doReturn(elementListAdapter)
        .when(adapterFactory)
        .createAdapterForElement(DEFAULT_TEMPLATE_ELEMENT, templateContext);

    TemplateKey templateKey = new TemplateKey(template, sharedStates, new ArrayList<>());
    when(elementListAdapter.getKey()).thenReturn(templateKey);
    templateRecyclerPool.put(templateKey, elementListAdapter);

    // Create adapter and ensure template key is set.
    ElementAdapter<? extends View, ?> adapter =
        templateBinder.createAndBindTemplateAdapter(model, frameContext);
    assertThat(adapter).isSameInstanceAs(elementListAdapter);

    // We don't get the adapter from the factory
    verifyZeroInteractions(adapterFactory);
    verify(elementListAdapter, never()).setKey(any(TemplateKey.class));

    // Assert that adapter is bound with the template frameContext
    verify(elementListAdapter).bindModel(model.getTemplate().getElement(), templateContext);
  }

  @Test
  public void testTemplateKey_equalWithSameObjects() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    Stylesheet stylesheet = Stylesheet.newBuilder().setStylesheetId("S").build();
    List<PietSharedState> sharedStates =
        listOfSharedStates(PietSharedState.newBuilder().addTemplates(template).build());
    List<Stylesheet> stylesheets = Collections.singletonList(stylesheet);
    TemplateKey key1 = new TemplateKey(template, sharedStates, stylesheets);
    TemplateKey key2 = new TemplateKey(template, sharedStates, stylesheets);

    assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    assertThat(key1).isEqualTo(key2);
  }

  @Test
  public void testTemplateKey_equalWithDifferentTemplateObject() {
    Template template1 = Template.newBuilder().setTemplateId("T").build();
    Template template2 = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template1).build();
    TemplateKey key1 =
        new TemplateKey(template1, listOfSharedStates(sharedState), new ArrayList<>());
    TemplateKey key2 =
        new TemplateKey(template2, listOfSharedStates(sharedState), new ArrayList<>());

    assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    assertThat(key1).isEqualTo(key2);
  }

  @Test
  public void testTemplateKey_equalWithDifferentSharedStateObject() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState1 = PietSharedState.newBuilder().addTemplates(template).build();
    PietSharedState sharedState2 = PietSharedState.newBuilder().addTemplates(template).build();
    TemplateKey key1 =
        new TemplateKey(template, listOfSharedStates(sharedState1), new ArrayList<>());
    TemplateKey key2 =
        new TemplateKey(template, listOfSharedStates(sharedState2), new ArrayList<>());

    assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    assertThat(key1).isEqualTo(key2);
  }

  @Test
  public void testTemplateKey_equalWithDifferentStylesheetObject() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template).build();
    Stylesheet stylesheet1 = Stylesheet.newBuilder().setStylesheetId("1").build();
    Stylesheet stylesheet2 = Stylesheet.newBuilder().setStylesheetId("1").build();
    TemplateKey key1 =
        new TemplateKey(
            template, listOfSharedStates(sharedState), Collections.singletonList(stylesheet1));
    TemplateKey key2 =
        new TemplateKey(
            template, listOfSharedStates(sharedState), Collections.singletonList(stylesheet2));

    assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    assertThat(key1).isEqualTo(key2);
  }

  @Test
  public void testTemplateKey_differentWithDifferentLengthSharedStates() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState1 = PietSharedState.newBuilder().addTemplates(template).build();
    TemplateKey key1 =
        new TemplateKey(template, listOfSharedStates(sharedState1), new ArrayList<>());
    TemplateKey key2 =
        new TemplateKey(
            template, listOfSharedStates(sharedState1, sharedState1), new ArrayList<>());

    assertThat(key1.hashCode()).isNotEqualTo(key2.hashCode());
    assertThat(key1).isNotEqualTo(key2);
  }

  @Test
  public void testTemplateKey_differentWithDifferentTemplate() {
    Template template1 = Template.newBuilder().setTemplateId("T1").build();
    Template template2 = Template.newBuilder().setTemplateId("T2").build();
    List<PietSharedState> sharedStates =
        listOfSharedStates(PietSharedState.newBuilder().addTemplates(template1).build());
    TemplateKey key1 = new TemplateKey(template1, sharedStates, new ArrayList<>());
    TemplateKey key2 = new TemplateKey(template2, sharedStates, new ArrayList<>());

    assertThat(key1.hashCode()).isNotEqualTo(key2.hashCode());
    assertThat(key1).isNotEqualTo(key2);
  }

  @Test
  public void testTemplateKey_differentWithDifferentSharedState() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState1 = PietSharedState.newBuilder().addTemplates(template).build();
    PietSharedState sharedState2 = PietSharedState.getDefaultInstance();
    TemplateKey key1 =
        new TemplateKey(template, listOfSharedStates(sharedState1), new ArrayList<>());
    TemplateKey key2 =
        new TemplateKey(template, listOfSharedStates(sharedState2), new ArrayList<>());

    assertThat(key1.hashCode()).isNotEqualTo(key2.hashCode());
    assertThat(key1).isNotEqualTo(key2);
  }

  @Test
  public void testTemplateKey_differentWithDifferentStylesheet() {
    Template template = Template.newBuilder().setTemplateId("T").build();
    PietSharedState sharedState = PietSharedState.newBuilder().addTemplates(template).build();
    Stylesheet stylesheet1 = Stylesheet.newBuilder().setStylesheetId("1").build();
    Stylesheet stylesheet2 = Stylesheet.newBuilder().setStylesheetId("2").build();
    TemplateKey key1 =
        new TemplateKey(
            template, listOfSharedStates(sharedState), Collections.singletonList(stylesheet1));
    TemplateKey key2 =
        new TemplateKey(
            template, listOfSharedStates(sharedState), Collections.singletonList(stylesheet2));

    assertThat(key1.hashCode()).isNotEqualTo(key2.hashCode());
    assertThat(key1).isNotEqualTo(key2);
  }

  @Test
  public void testTemplateAdapterModel_getters() {
    TemplateAdapterModel model = new TemplateAdapterModel(MODEL_TEMPLATE, MODEL_BINDING_CONTEXT);
    assertThat(model.getTemplate()).isSameInstanceAs(MODEL_TEMPLATE);
    assertThat(model.getBindingContext()).isSameInstanceAs(MODEL_BINDING_CONTEXT);
  }

  @Test
  public void testTemplateAdapterModel_lookUpTemplate() {
    ActionHandler actionHandler = mock(ActionHandler.class);
    List<PietSharedState> pietSharedStates = Collections.emptyList();
    Context context = Robolectric.buildActivity(Activity.class).get();

    HostProviders mockHostProviders = mock(HostProviders.class);
    AssetProvider mockAssetProvider = mock(AssetProvider.class);
    when(mockHostProviders.getAssetProvider()).thenReturn(mockAssetProvider);

    MediaQueryHelper mediaQueryHelper =
        new MediaQueryHelper(FRAME_WIDTH_PX, mockAssetProvider, context);
    PietStylesHelper pietStylesHelper =
        new PietStylesHelperFactory().get(pietSharedStates, mediaQueryHelper);

    FrameContext frameContext =
        FrameContext.createFrameContext(
            DEFAULT_FRAME, // This defines MODEL_TEMPLATE
            pietSharedStates,
            pietStylesHelper,
            DebugBehavior.VERBOSE,
            new DebugLogger(),
            actionHandler,
            mockHostProviders,
            new FrameLayout(context));

    TemplateAdapterModel model =
        new TemplateAdapterModel(
            MODEL_TEMPLATE.getTemplateId(), frameContext, MODEL_BINDING_CONTEXT);
    assertThat(model.getTemplate()).isSameInstanceAs(MODEL_TEMPLATE);
    assertThat(model.getBindingContext()).isSameInstanceAs(MODEL_BINDING_CONTEXT);
  }

  @Test
  public void testTemplateAdapterModel_equalsSame() {
    TemplateAdapterModel model1 = new TemplateAdapterModel(MODEL_TEMPLATE, MODEL_BINDING_CONTEXT);
    TemplateAdapterModel model2 = new TemplateAdapterModel(MODEL_TEMPLATE, MODEL_BINDING_CONTEXT);
    assertThat(model1).isEqualTo(model2);
    assertThat(model1.hashCode()).isEqualTo(model2.hashCode());
  }

  @Test
  public void testTemplateAdapterModel_equalsOtherInstance() {
    TemplateAdapterModel model1 = new TemplateAdapterModel(MODEL_TEMPLATE, MODEL_BINDING_CONTEXT);
    TemplateAdapterModel model2 =
        new TemplateAdapterModel(
            MODEL_TEMPLATE.toBuilder().build(), MODEL_BINDING_CONTEXT.toBuilder().build());
    assertThat(model1.getTemplate()).isNotSameInstanceAs(model2.getTemplate());
    assertThat(model1.getBindingContext()).isNotSameInstanceAs(model2.getBindingContext());
    assertThat(model1).isEqualTo(model2);
    assertThat(model1.hashCode()).isEqualTo(model2.hashCode());
  }

  @Test
  public void testTemplateAdapterModel_notEquals() {
    TemplateAdapterModel model1 = new TemplateAdapterModel(MODEL_TEMPLATE, MODEL_BINDING_CONTEXT);
    TemplateAdapterModel model2 =
        new TemplateAdapterModel(
            MODEL_TEMPLATE.toBuilder().clearTemplateId().build(), MODEL_BINDING_CONTEXT);
    TemplateAdapterModel model3 =
        new TemplateAdapterModel(
            MODEL_TEMPLATE, MODEL_BINDING_CONTEXT.toBuilder().clearBindingValues().build());
    assertThat(model1).isNotEqualTo(model2);
    assertThat(model1.hashCode()).isNotEqualTo(model2.hashCode());
    assertThat(model1).isNotEqualTo(model3);
    assertThat(model1.hashCode()).isNotEqualTo(model3.hashCode());
  }

  private List<PietSharedState> listOfSharedStates(PietSharedState... pietSharedStates) {
    List<PietSharedState> sharedStates = new ArrayList<>();
    Collections.addAll(sharedStates, pietSharedStates);
    return sharedStates;
  }
}
